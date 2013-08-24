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

#include "openfimg_drmif.h"
#include "openfimg_priv.h"

struct of_pipe * of_pipe_new(struct of_device *dev, enum of_pipe_id id)
{
	struct of_pipe *pipe;
	int fd;

	fd = drmOpen("exynos", NULL);
	if(fd < 0) {
		ERROR_MSG("Couldn't open pipe: %s", strerror(errno));
		return NULL;
	}

	pipe = calloc(1, sizeof(*pipe));
	if (!pipe) {
		close(fd);
		ERROR_MSG("allocation failed");
		return NULL;
	}

	pipe->dev = dev;
	pipe->id = id;
	pipe->fd = fd;

	list_inithead(&pipe->submit_list);
	list_inithead(&pipe->pending_list);

	return pipe;
}

void of_pipe_del(struct of_pipe *pipe)
{
	if (pipe->fd)
		close(pipe->fd);

	free(pipe);
}

int of_pipe_wait(struct of_pipe *pipe, uint32_t timestamp)
{
	/* TODO */
	return -EINVAL;
}

int of_pipe_timestamp(struct of_pipe *pipe, uint32_t *timestamp)
{
	/* TODO */
	return -EINVAL;
}

/* add buffer to submit list when it is referenced in cmdstream: */
void of_pipe_add_submit(struct of_pipe *pipe, struct of_bo *bo)
{
	struct list_head *list = &bo->list[pipe->id];
	if (LIST_IS_EMPTY(list)) {
		of_bo_ref(bo);
	} else {
		list_del(list);
	}
	list_addtail(list, &pipe->submit_list);
}

/* prepare buffers on submit list before flush: */
void of_pipe_pre_submit(struct of_pipe *pipe)
{
	struct of_bo *bo = NULL;

	if (pipe->id == OF_PIPE_3D)
		return;

	if (!pipe->p3d)
		pipe->p3d = of_pipe_new(pipe->dev, OF_PIPE_3D);

	LIST_FOR_EACH_ENTRY(bo, &pipe->submit_list, list[pipe->id]) {
		uint32_t timestamp = of_bo_get_timestamp(bo);
		if (timestamp)
			of_pipe_wait(pipe->p3d, timestamp);
	}
}

/* process buffers on submit list after flush: */
void of_pipe_post_submit(struct of_pipe *pipe, uint32_t timestamp)
{
	struct of_bo *bo = NULL, *tmp;

	LIST_FOR_EACH_ENTRY_SAFE(bo, tmp, &pipe->submit_list, list[pipe->id]) {
		struct list_head *list = &bo->list[pipe->id];
		list_del(list);
		bo->timestamp[pipe->id] = timestamp;
		list_addtail(list, &pipe->pending_list);

		if (pipe->id == OF_PIPE_3D)
			fb_bo_set_timestamp(bo, timestamp);
	}

	if (!of_pipe_timestamp(pipe, &timestamp))
		of_pipe_process_pending(pipe, timestamp);
}

void of_pipe_process_pending(struct of_pipe *pipe, uint32_t timestamp)
{
	struct of_bo *bo = NULL, *tmp;

	LIST_FOR_EACH_ENTRY_SAFE(bo, tmp, &pipe->pending_list, list[pipe->id]) {
		struct list_head *list = &bo->list[pipe->id];
		if (bo->timestamp[pipe->id] > timestamp)
			return;
		list_delinit(list);
		bo->timestamp[pipe->id] = 0;
		of_bo_del(bo);
	}
}
