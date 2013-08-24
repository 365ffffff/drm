/*
 * Copyright (C) 2013 Tomasz Figa <tomasz.figa@gmail.com>
 *
 * Parts shamelessly copied from Freedreno driver:
 *
 * Copyright (C) 2012 Rob Clark <robclark@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <assert.h>

#include "openfimg_drmif.h"
#include "openfimg_priv.h"
#include "openfimg_ringbuffer.h"

struct of_rb_bo {
	struct of_pipe *pipe;
	struct of_bo *bo;
	void    *hostptr;
	uint32_t size;
};

struct of_ringmarker {
	struct of_ringbuffer *ring;
	uint32_t *cur;
};

static void of_rb_bo_del(struct of_rb_bo *bo)
{
	munmap(bo->hostptr, bo->size);
	of_bo_del(bo->bo);
	free(bo);
}

static struct of_rb_bo * of_rb_bo_new(struct of_pipe *pipe, uint32_t size)
{
	struct of_rb_bo *bo;

	bo = calloc(1, sizeof(*bo));
	if (!bo) {
		ERROR_MSG("allocation failed");
		return NULL;
	}

	bo->bo = of_bo_new(pipe->dev, size, 0);
	if (!bo->bo) {
		ERROR_MSG("BO allocation failed");
		goto fail;
	}

	bo->pipe = pipe;
	bo->size = size;
	bo->hostptr = of_bo_map(bo->bo);
	if (!bo->hostptr) {
		ERROR_MSG("failed to mmap BO");
		goto fail_mmap;
	}

	return bo;

fail_mmap:
	of_bo_del(bo->bo);
fail:
	free(bo);

	return NULL;
}

struct of_ringbuffer * of_ringbuffer_new(struct of_pipe *pipe,
		uint32_t size)
{
	struct of_ringbuffer *ring = NULL;

	ring = calloc(1, sizeof(*ring));
	if (!ring) {
		ERROR_MSG("allocation failed");
		goto fail;
	}

	ring->bo = of_rb_bo_new(pipe, size);
	if (!ring->bo) {
		ERROR_MSG("ringbuffer allocation failed");
		goto fail;
	}

	ring->size = size;
	ring->pipe = pipe;
	ring->start = ring->bo->hostptr;
	ring->end = &(ring->start[size/4]);

	ring->cur = ring->last_start = ring->start;

	return ring;
fail:
	if (ring)
		of_ringbuffer_del(ring);
	return NULL;
}

void of_ringbuffer_del(struct of_ringbuffer *ring)
{
	if (ring->bo)
		of_rb_bo_del(ring->bo);
	free(ring);
}

void of_ringbuffer_reset(struct of_ringbuffer *ring)
{
	uint32_t *start = ring->start;

	ring->cur = ring->last_start = start;
}

static int flush_impl(struct of_ringbuffer *ring, uint32_t *last_start)
{
	struct drm_exynos_g3d_submit req = {
		.handle = of_bo_handle(ring->bo->bo),
		.offset = (uint8_t *)last_start - (uint8_t *)ring->start,
		.size = (uint8_t *)ring->cur - (uint8_t *)last_start,
	};
	int ret;

	of_pipe_pre_submit(ring->pipe);

	ret = ioctl(ring->pipe->fd, DRM_IOCTL_EXYNOS_G3D_SUBMIT, &req);
	if (ret)
		ERROR_MSG("G3D_SUBMIT failed! %d (%s)", ret, strerror(errno));

	ring->last_timestamp = req.timestamp;
	ring->last_start = ring->cur;

	of_pipe_post_submit(ring->pipe, req.timestamp);

	return ret;
}

/* maybe get rid of this and use of_ringmarker_flush() from DDX too? */
int of_ringbuffer_flush(struct of_ringbuffer *ring)
{
	return flush_impl(ring, ring->last_start);
}

uint32_t of_ringbuffer_timestamp(struct of_ringbuffer *ring)
{
	return ring->last_timestamp;
}

void of_ringbuffer_emit_reloc(struct of_ringbuffer *ring,
		struct of_bo *bo, uint32_t offset, uint32_t or)
{
	uint32_t addr = of_bo_gpuaddr(bo, offset);
	assert(addr);
	(*ring->cur++) = addr | or;
	of_pipe_add_submit(ring->pipe, bo);
}

void of_ringbuffer_emit_reloc_shift(struct of_ringbuffer *ring,
		struct of_bo *bo, uint32_t offset, uint32_t or, int32_t shift)
{
	uint32_t addr = of_bo_gpuaddr(bo, offset);
	assert(addr);
	if (shift < 0)
		addr >>= -shift;
	else
		addr <<= shift;
	(*ring->cur++) = addr | or;
	of_pipe_add_submit(ring->pipe, bo);
}

void of_ringbuffer_emit_reloc_ring(struct of_ringbuffer *ring,
		struct of_ringmarker *target)
{
	(*ring->cur++) = (unsigned long)target->cur;
}

struct of_ringmarker * of_ringmarker_new(struct of_ringbuffer *ring)
{
	struct of_ringmarker *marker = NULL;

	marker = calloc(1, sizeof(*marker));
	if (!marker) {
		ERROR_MSG("allocation failed");
		return NULL;
	}

	marker->ring = ring;

	of_ringmarker_mark(marker);

	return marker;
}

void of_ringmarker_del(struct of_ringmarker *marker)
{
	free(marker);
}

void of_ringmarker_mark(struct of_ringmarker *marker)
{
	marker->cur = marker->ring->cur;
}

uint32_t of_ringmarker_dwords(struct of_ringmarker *start,
		struct of_ringmarker *end)
{
	return end->cur - start->cur;
}

int of_ringmarker_flush(struct of_ringmarker *marker)
{
	return flush_impl(marker->ring, marker->cur);
}
