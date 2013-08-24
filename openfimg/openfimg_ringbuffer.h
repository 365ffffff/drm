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

#ifndef FREEDRENO_RINGBUFFER_H_
#define FREEDRENO_RINGBUFFER_H_

#include <openfimg_drmif.h>

/* the ringbuffer object is not opaque so that OUT_RING() type stuff
 * can be inlined.  Note that users should not make assumptions about
 * the size of this struct.. more stuff will be added when we eventually
 * have a kernel driver that can deal w/ reloc's..
 */

struct of_rb_bo;
struct of_ringmarker;

struct of_ringbuffer {
	int size;
	uint32_t *cur, *end, *start, *last_start;
	struct of_pipe *pipe;
	struct of_rb_bo *bo;
	uint32_t last_timestamp;
};

/* ringbuffer flush flags:
 *   SAVE_GMEM - GMEM contents not preserved to system memory
 *       in cmds flushed so if there is a context switch after
 *       this flush and before the next one the kernel must
 *       save GMEM contents
 *   SUBMIT_IB_LIST - tbd..
 */
#define DRM_FREEDRENO_CONTEXT_SAVE_GMEM       1
#define DRM_FREEDRENO_CONTEXT_SUBMIT_IB_LIST  4


struct of_ringbuffer * of_ringbuffer_new(struct of_pipe *pipe,
		uint32_t size);
void of_ringbuffer_del(struct of_ringbuffer *ring);
void of_ringbuffer_reset(struct of_ringbuffer *ring);
int of_ringbuffer_flush(struct of_ringbuffer *ring);
uint32_t of_ringbuffer_timestamp(struct of_ringbuffer *ring);

static inline void of_ringbuffer_emit(struct of_ringbuffer *ring,
		uint32_t data)
{
	(*ring->cur++) = data;
}

void of_ringbuffer_emit_reloc(struct of_ringbuffer *ring,
		struct of_bo *bo, uint32_t offset, uint32_t or);
void of_ringbuffer_emit_reloc_shift(struct of_ringbuffer *ring,
		struct of_bo *bo, uint32_t offset, uint32_t or, int32_t shift);
void of_ringbuffer_emit_reloc_ring(struct of_ringbuffer *ring,
		struct of_ringmarker *target);

struct of_ringmarker * of_ringmarker_new(struct of_ringbuffer *ring);
void of_ringmarker_del(struct of_ringmarker *marker);
void of_ringmarker_mark(struct of_ringmarker *marker);
uint32_t of_ringmarker_dwords(struct of_ringmarker *start,
		struct of_ringmarker *end);
int of_ringmarker_flush(struct of_ringmarker *marker);

#endif /* FREEDRENO_RINGBUFFER_H_ */
