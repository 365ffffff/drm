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

#ifndef FREEDRENO_DRMIF_H_
#define FREEDRENO_DRMIF_H_

#include <xf86drm.h>
#include <stdint.h>

struct of_bo;
struct of_pipe;
struct of_device;

enum of_pipe_id {
	OF_PIPE_3D,
	/* Only 3D pipe is exposed as of now. */
	OF_PIPE_MAX,
};

/* bo flags: */
#define DRM_OPENFIMG_GEM_NONCONTIG	(1 << 0)
#define DRM_OPENFIMG_GEM_WBACK		(1 << 1)
#define DRM_OPENFIMG_GEM_WCOMBINE	(1 << 2)
#define DRM_OPENFIMG_GEM_UNMAPPED	(1 << 3)

/* device functions:
 */

struct of_device * of_device_new(int fd);
struct of_device * of_device_ref(struct of_device *dev);
void of_device_del(struct of_device *dev);


/* pipe functions:
 */

struct of_pipe * of_pipe_new(struct of_device *dev, enum of_pipe_id id);
void of_pipe_del(struct of_pipe *pipe);
int of_pipe_wait(struct of_pipe *pipe, uint32_t timestamp);
int of_pipe_timestamp(struct of_pipe *pipe, uint32_t *timestamp);


/* buffer-object functions:
 */

struct of_bo * of_bo_new(struct of_device *dev,
		uint32_t size, uint32_t flags);
struct of_bo * of_bo_from_name(struct of_device *dev, uint32_t name);
struct of_bo * of_bo_ref(struct of_bo *bo);
void of_bo_del(struct of_bo *bo);
int of_bo_get_name(struct of_bo *bo, uint32_t *name);
uint32_t of_bo_handle(struct of_bo *bo);
uint32_t of_bo_size(struct of_bo *bo);
void * of_bo_map(struct of_bo *bo);

#endif /* FREEDRENO_DRMIF_H_ */
