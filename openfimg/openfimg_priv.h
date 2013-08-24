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

#ifndef FREEDRENO_PRIV_H_
#define FREEDRENO_PRIV_H_

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <pthread.h>

#include "xf86drm.h"
#include "xf86atomic.h"

#include "list.h"

#include "exynos_drm.h"
#include "openfimg_drmif.h"

struct of_device {
	int fd;
	atomic_t refcnt;

	/* tables to keep track of bo's, to avoid "evil-twin" of_bo objects:
	 *
	 *   handle_table: maps handle to of_bo
	 *   name_table: maps flink name to of_bo
	 *
	 * We end up needing two tables, because DRM_IOCTL_GEM_OPEN always
	 * returns a new handle.  So we need to figure out if the bo is already
	 * open in the process first, before calling gem-open.
	 */
	void *handle_table, *name_table;
};

struct of_pipe {
	struct of_device *dev;
	enum of_pipe_id id;
	int fd;

	/* list of bo's that are referenced in ringbuffer but not
	 * submitted yet:
	 */
	struct list_head submit_list;

	/* list of bo's that have been submitted but timestamp has
	 * not passed yet (so still ref'd in active cmdstream)
	 */
	struct list_head pending_list;

	/* if we are the 2d pipe, and want to wait on a timestamp
	 * from 3d, we need to also internally open the 3d pipe:
	 */
	struct of_pipe *p3d;
};

void of_pipe_add_submit(struct of_pipe *pipe, struct of_bo *bo);
void of_pipe_pre_submit(struct of_pipe *pipe);
void of_pipe_post_submit(struct of_pipe *pipe, uint32_t timestamp);
void of_pipe_process_pending(struct of_pipe *pipe, uint32_t timestamp);

struct of_bo {
	struct of_device *dev;
	uint32_t size;
	uint32_t handle;
	uint32_t name;
	uint32_t gpuaddr;
	void *map;
	uint64_t offset;
	/* timestamp (per pipe) for bo's in a pipe's pending_list: */
	uint32_t timestamp[OF_PIPE_MAX];
	/* list-node for pipe's submit_list or pending_list */
	struct list_head list[OF_PIPE_MAX];
	atomic_t refcnt;
};

/* not exposed publicly, because won't be needed when we have
 * a proper kernel driver
 */
uint32_t of_bo_gpuaddr(struct of_bo *bo, uint32_t offset);
void fb_bo_set_timestamp(struct of_bo *bo, uint32_t timestamp);
uint32_t of_bo_get_timestamp(struct of_bo *bo);

#define ALIGN(v,a) (((v) + (a) - 1) & ~((a) - 1))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define enable_debug 1  /* TODO make dynamic */

#define INFO_MSG(fmt, ...) \
		do { drmMsg("[I] "fmt " (%s:%d)\n", \
				##__VA_ARGS__, __FUNCTION__, __LINE__); } while (0)
#define DEBUG_MSG(fmt, ...) \
		do if (enable_debug) { drmMsg("[D] "fmt " (%s:%d)\n", \
				##__VA_ARGS__, __FUNCTION__, __LINE__); } while (0)
#define WARN_MSG(fmt, ...) \
		do { drmMsg("[W] "fmt " (%s:%d)\n", \
				##__VA_ARGS__, __FUNCTION__, __LINE__); } while (0)
#define ERROR_MSG(fmt, ...) \
		do { drmMsg("[E] " fmt " (%s:%d)\n", \
				##__VA_ARGS__, __FUNCTION__, __LINE__); } while (0)

#endif /* FREEDRENO_PRIV_H_ */
