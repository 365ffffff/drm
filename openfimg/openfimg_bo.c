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

#include <linux/fb.h>

static pthread_mutex_t table_lock = PTHREAD_MUTEX_INITIALIZER;

/* set buffer name, and add to table, call w/ table_lock held: */
static void set_name(struct of_bo *bo, uint32_t name)
{
	bo->name = name;
	/* add ourself into the handle table: */
	drmHashInsert(bo->dev->name_table, name, bo);
}

/* lookup a buffer, call w/ table_lock held: */
static struct of_bo * lookup_bo(void *tbl, uint32_t key)
{
	struct of_bo *bo = NULL;

	if (!drmHashLookup(tbl, key, (void **)&bo)) {
		/* found, incr refcnt and return: */
		bo = of_bo_ref(bo);
	}

	return bo;
}

/* allocate a new buffer object, call w/ table_lock held */
static struct of_bo * bo_from_handle(struct of_device *dev,
		uint32_t size, uint32_t handle)
{
	struct of_bo *bo = calloc(1, sizeof(*bo));
	unsigned i;

	if (!bo) {
		struct drm_gem_close req = {
				.handle = handle,
		};
		drmIoctl(dev->fd, DRM_IOCTL_GEM_CLOSE, &req);
		return NULL;
	}

	bo->dev = of_device_ref(dev);
	bo->size = size;
	bo->handle = handle;
	atomic_set(&bo->refcnt, 1);

	/* add ourself into the handle table: */
	drmHashInsert(dev->handle_table, handle, bo);
	for (i = 0; i < ARRAY_SIZE(bo->list); i++)
		list_inithead(&bo->list[i]);

	return bo;
}

struct of_bo * of_bo_new(struct of_device *dev,
		uint32_t size, uint32_t flags)
{
	struct drm_exynos_gem_create req = {
		.size = ALIGN(size, 4096),
		.flags = flags,
	};
	struct of_bo *bo = NULL;

	if (drmIoctl(dev->fd, DRM_IOCTL_EXYNOS_GEM_CREATE, &req)){
		ERROR_MSG("failed to create gem object: %s",
				strerror(errno));
		return NULL;
	}

	pthread_mutex_lock(&table_lock);
	bo = bo_from_handle(dev, size, req.handle);
	pthread_mutex_unlock(&table_lock);
	if (!bo) {
		ERROR_MSG("bo_from_handle failed");
		return NULL;
	}

	return bo;
}

struct of_bo * of_bo_from_name(struct of_device *dev, uint32_t name)
{
	struct drm_gem_open req = {
			.name = name,
	};
	struct of_bo *bo;

	pthread_mutex_lock(&table_lock);

	/* check name table first, to see if bo is already open: */
	bo = lookup_bo(dev->name_table, name);
	if (bo)
		goto out_unlock;

	if (drmIoctl(dev->fd, DRM_IOCTL_GEM_OPEN, &req)) {
		ERROR_MSG("gem-open failed: %s", strerror(errno));
		goto out_unlock;
	}

	bo = lookup_bo(dev->handle_table, req.handle);
	if (bo)
		goto out_unlock;

	bo = bo_from_handle(dev, req.size, req.handle);
	if (bo)
		set_name(bo, name);

out_unlock:
	pthread_mutex_unlock(&table_lock);

	return bo;
}

struct of_bo * of_bo_ref(struct of_bo *bo)
{
	atomic_inc(&bo->refcnt);
	return bo;
}

void of_bo_del(struct of_bo *bo)
{
	if (!atomic_dec_and_test(&bo->refcnt))
		return;

	if (bo->map)
		munmap(bo->map, bo->size);

	if (bo->handle) {
		struct drm_gem_close req = {
				.handle = bo->handle,
		};
		pthread_mutex_lock(&table_lock);
		drmHashDelete(bo->dev->handle_table, bo->handle);
		if (bo->name)
			drmHashDelete(bo->dev->name_table, bo->name);
		drmIoctl(bo->dev->fd, DRM_IOCTL_GEM_CLOSE, &req);
		pthread_mutex_unlock(&table_lock);
	}

	of_device_del(bo->dev);
	free(bo);
}

int of_bo_get_name(struct of_bo *bo, uint32_t *name)
{
	if (!bo->name) {
		struct drm_gem_flink req = {
				.handle = bo->handle,
		};
		int ret;

		ret = drmIoctl(bo->dev->fd, DRM_IOCTL_GEM_FLINK, &req);
		if (ret) {
			return ret;
		}

		pthread_mutex_lock(&table_lock);
		set_name(bo, req.name);
		pthread_mutex_unlock(&table_lock);
	}

	*name = bo->name;

	return 0;
}

uint32_t of_bo_handle(struct of_bo *bo)
{
	return bo->handle;
}

uint32_t of_bo_size(struct of_bo *bo)
{
	return bo->size;
}

void * of_bo_map(struct of_bo *bo)
{
	if (!bo->map) {
		struct drm_exynos_gem_mmap req = {
			.handle = bo->handle,
			.size	= bo->size,
		};
		int ret;

		ret = drmIoctl(bo->dev->fd, DRM_IOCTL_EXYNOS_GEM_MMAP, &req);
		if (ret) {
			ERROR_MSG("mmap failed: %s", strerror(errno));
			bo->map = NULL;
			return NULL;
		}

		bo->map = (void *)(unsigned long)req.mapped;
	}
	return bo->map;
}
