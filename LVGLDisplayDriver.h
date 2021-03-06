/* LittlevGL for Mbed-OS library
 * Copyright (c) 2018-2019 George "AGlass0fMilk" Beckstein
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MBED_LVGL_DRIVERS_LVGLDISPLAYDRIVER_H_
#define MBED_LVGL_DRIVERS_LVGLDISPLAYDRIVER_H_

#include "lv_color.h"
#include "lv_area.h"
#include "lv_hal_disp.h"

#include "platform/Span.h"

class LVGLDisplayDriver
{

public:

	// Declare LittlevGL a friend class
	friend class LittlevGL;

		/**
		 * Constructor for LVGLDisplayDriver
		 *
		 * @param[in] primary_display_buffer (optional) The user may provide a display buffer to use (or one will be dynamically allocated)
		 * @param[in] secondary_display_buffer (optional) If using a double-buffered scheme, the user must provide both display buffers
		 *
		 * @note For double-buffered operation, the display buffers MUST be the same size!
		 */
		LVGLDisplayDriver(mbed::Span<lv_color_t> primary_display_buffer = mbed::Span<lv_color_t, 0>(),
				mbed::Span<lv_color_t> secondary_display_buffer = mbed::Span<lv_color_t, 0>()) :
				hor_res(LV_HOR_RES_MAX), ver_res(LV_VER_RES_MAX), lv_disp_obj(NULL) {

			// If the user doesn't provide a display buffer to use, dynamically allocate one
			if(primary_display_buffer.empty()) {
				user_provided_display_buffer = false;
				lv_color_t* buf = new lv_color_t[MBED_CONF_MBED_LVGL_DEFAULT_DISPLAY_BUFFER_SIZE];
				this->primary_display_buffer = mbed::Span<lv_color_t>(buf, MBED_CONF_MBED_LVGL_DEFAULT_DISPLAY_BUFFER_SIZE);
			} else {
				user_provided_display_buffer = true;
				this->primary_display_buffer = primary_display_buffer;

				// Make sure the secondary display buffer is the same size or not used
				MBED_ASSERT(secondary_display_buffer.empty() ||
							secondary_display_buffer.size() == primary_display_buffer.size());
			}

			// Secondary display buffer is only used if the user provides one
			this->secondary_display_buffer = secondary_display_buffer;

			// Fill out the lv_disp_buf struct
			initialize_display_buffers();
		}

		virtual ~LVGLDisplayDriver() {
			// Clean up our dynamically allocated display buffer
			if(!user_provided_display_buffer) {
				delete[] primary_display_buffer.data();
			}
		}

		/**
		 * Sets the display's resolution
		 *
		 * @param[in] new_hor_res New horizontal resolution
		 * @param[in] new_ver_res New vertical resolution
		 *
		 * @note should be set before the display driver is registered with
		 * LittlevGL
		 *
		 * @note The horizontal and vertical resolutions must be below
		 * the maximums set in your configuration
		 */
		void set_resolution(lv_coord_t new_hor_res, lv_coord_t new_ver_res) {
			MBED_ASSERT(new_hor_res <= LV_HOR_RES_MAX);
			MBED_ASSERT(new_ver_res <= LV_VER_RES_MAX);
			hor_res = new_hor_res;
			ver_res = new_ver_res;
		}

		/**
		 * Gets the display's resolution
		 *
		 * @param[out] hor_res Horizontal resolution of display
		 * @param[out] ver_res Vertical resolution of display
		 */
		void get_resolution(lv_coord_t* hor_res, lv_coord_t* ver_res) {
			*hor_res = this->hor_res;
			*ver_res = this->ver_res;
		}

/* TODO - figure out why making LittlevGL a friend class does not
 * allow access to protected member functions...
 *
 * It is understandable for virtual methods but still doesn't work even
 * for concrete methods! Compiler issue?
 */
//protected:

		/*
		 * @brief Flush the content of the internal buffer to the specific area on the display
		 * You can use DMA or any hardware acceleration to do this operation in the background but
		 * this function must block the caller until the transfer is complete!
		 */
		virtual void flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p) = 0;

		/**
		 * Subclass returns true if it has a custom rounder function
		 */
		virtual bool has_rounder(void) {
			return false;
		}

		/** OPTIONAL: Extend the invalidated areas to match with the display drivers requirements
		 * E.g. round `y` to, 8, 16 ..) on a monochrome display*/
		virtual void round_lv_area(lv_disp_drv_t * disp_drv, lv_area_t * area) { }

		/**
		 * Subclass returns true if it has a custom pixel write function
		 */
		virtual bool has_pix_write_func(void) {
			return false;
		}

		/*Optional: Set a pixel in a buffer according to the requirements of the display*/
		virtual void set_pixel(lv_disp_drv_t * disp_drv, uint8_t * buf, lv_coord_t buf_w, lv_coord_t x, lv_coord_t y,
                lv_color_t color, lv_opa_t opa) { }

		/** OPTIONAL: Called after every refresh cycle to tell the rendering and flushing time + the
		 * number of flushed pixels */
		virtual void monitor(lv_disp_drv_t * disp_drv, uint32_t time, uint32_t px) {
#if MBED_CONF_MBED_LVGL_ENABLE_FLUSH_MONITORING
			printf("%d px refreshed in %d ms\n", time, ms);
#endif
		}

#if USE_LV_GPU

		/** OPTIONAL: Blend two memories using opacity (GPU only)*/
		virtual void gpu_blend(lv_disp_drv_t * disp_drv, lv_color_t * dest, const lv_color_t * src, uint32_t length,
							 lv_opa_t opa) { }

		/** OPTIONAL: Fill a memory with a color (GPU only)*/
		virtual void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
							const lv_area_t * fill_area, lv_color_t color) { }

#endif

// TODO - see above comment about friend declaration not working
//protected:

		lv_disp_buf_t* get_lv_buf(void) {
			return &lv_buf;
		}

		void set_lv_disp_obj(lv_disp_t* disp_obj) {
			lv_disp_obj = disp_obj;
		}

		/**
		 * Gets the display's underlying LVGL handle
		 *
		 * @retval pointer to lvgl display handle
		 */
		lv_disp_t* get_lv_disp_obj(void) {
			return lv_disp_obj;
		}

protected:

		/**
		 * Internal function to initialize the underlying LittlevGL
		 * display buffer structure
		 */
		void initialize_display_buffers(void) {
			lv_disp_buf_init(&this->lv_buf, this->primary_display_buffer.data(),
					this->secondary_display_buffer.data(), this->primary_display_buffer.size());
		}

protected:

		/** Primary display buffer */
		mbed::Span<lv_color_t> primary_display_buffer;

		/** Secondary display buffer for double-buffered display schemes */
		mbed::Span<lv_color_t> secondary_display_buffer;

		lv_coord_t hor_res; /** Horizontal resolution */
		lv_coord_t ver_res; /** Vertical resolution */

private:

		/** C struct for interfacing to LVGL */
		lv_disp_buf_t lv_buf;

		/** Keep track of who owns the display buffer */
		bool user_provided_display_buffer;

		/** C struct for accessing LVGL display object */
		lv_disp_t* lv_disp_obj;

};



#endif /* MBED_LVGL_DRIVERS_LVGLDISPLAYDRIVER_H_ */
