/*
 * Copyright (C) 2000-2003 the xine project
 *
 * This file is part of xine, a free video player.
 *
 * xine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * xine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 * network buffering control
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>

/********** logging **********/
#define LOG_MODULE "net_buf_ctrl"
#define LOG_VERBOSE
/*
#define LOG
*/


#include "net_buf_ctrl.h"

#define FULL_FIFO_MARK             5 /* buffers free */

#define FIFO_PUT                   0
#define FIFO_GET                   1


static void report_progress (xine_stream_t *stream, int p) {

  xine_event_t             event;
  xine_progress_data_t     prg;

  prg.description = _("Buffering...");
  prg.percent = (p>100)?100:p;

  event.type = XINE_EVENT_PROGRESS;
  event.data = &prg;
  event.data_length = sizeof (xine_progress_data_t);

  xine_event_send (stream, &event);
}

void nbc_set_speed_pause (nbc_t *this) {
  xine_stream_t *stream = this->stream;

  xprintf(stream->xine, XINE_VERBOSITY_DEBUG, "\nnet_buf_ctrl: nbc_set_speed_pause\n");
  _x_set_speed (stream, XINE_SPEED_PAUSE);
  stream->xine->clock->set_option (stream->xine->clock, CLOCK_SCR_ADJUSTABLE, 0);
}

void nbc_set_speed_normal (nbc_t *this) {
  xine_stream_t *stream = this->stream;

  xprintf(stream->xine, XINE_VERBOSITY_DEBUG, "\nnet_buf_ctrl: nbc_set_speed_normal\n");
  _x_set_speed (stream, XINE_SPEED_NORMAL);
  stream->xine->clock->set_option (stream->xine->clock, CLOCK_SCR_ADJUSTABLE, 1);
}

static void display_stats (nbc_t *this) {
  static const char buffering[2][4] = {"   ", "buf"};
  static const char enabled[2][4]   = {"off", "on "};

  printf("net_buf_ctrl: vid %3d%% %4.1fs %4" PRId64 "kbps %1d, "\
	 "aud %3d%% %4.1fs %4" PRId64 "kbps %1d, %s %s%c",
	 this->video_fifo_fill,
	 (float)(this->video_fifo_length / 1000),
	 this->video_br / 1000,
	 this->video_in_disc,
	 this->audio_fifo_fill,
	 (float)(this->audio_fifo_length / 1000),
	 this->audio_br / 1000,
	 this->audio_in_disc,
	 buffering[this->buffering],
	 enabled[this->enabled],
	 isatty (STDOUT_FILENO) ? '\r' : '\n'
	 );
  fflush(stdout);
}

static void report_stats (nbc_t *this, int type) {
  xine_event_t             event;
  xine_nbc_stats_data_t    bs;

  bs.v_percent = this->video_fifo_fill;
  bs.v_remaining = this->video_fifo_length;
  bs.v_bitrate = this->video_br;
  bs.v_in_disc = this->video_in_disc;
  bs.a_percent = this->audio_fifo_fill;
  bs.a_remaining = this->audio_fifo_length;
  bs.a_bitrate = this->audio_br;
  bs.a_in_disc = this->audio_in_disc;
  bs.buffering = this->buffering;
  bs.enabled = this->enabled;
  bs.type = type;

  event.type = XINE_EVENT_NBC_STATS;
  event.data = &bs;
  event.data_length = sizeof (xine_nbc_stats_data_t);

  xine_event_send (this->stream, &event);
}

/*  Try to compute the length of the fifo in 1/1000 s
 *  2 methods :
 *    if the bitrate is known
 *      use the size of the fifo
 *    else
 *      use the the first and the last pts of the fifo
 */
void nbc_compute_fifo_length(nbc_t *this,
                                    fifo_buffer_t *fifo,
                                    buf_element_t *buf,
                                    int action) {
  int fifo_free, fifo_fill, fifo_div;
  int64_t video_br, audio_br, diff;
  int has_video, has_audio;

  has_video = _x_stream_info_get(this->stream, XINE_STREAM_INFO_HAS_VIDEO);
  has_audio = _x_stream_info_get(this->stream, XINE_STREAM_INFO_HAS_AUDIO);
  video_br  = _x_stream_info_get(this->stream, XINE_STREAM_INFO_VIDEO_BITRATE);
  audio_br  = _x_stream_info_get(this->stream, XINE_STREAM_INFO_AUDIO_BITRATE);

  fifo_free = fifo->buffer_pool_num_free;
  fifo_fill = fifo->fifo_size;
  fifo_div = fifo_fill + fifo_free - 1;
  if (fifo_div == 0)
    fifo_div = 1; /* avoid a possible divide-by-zero */

  if (fifo == this->video_fifo) {
    this->video_fifo_free = fifo_free;
    this->video_fifo_fill = (100 * fifo_fill) / fifo_div;
    this->video_fifo_size = fifo->fifo_data_size;

    if (buf->pts && (this->video_in_disc == 0)) {
      if (action == FIFO_PUT) {
        this->video_last_pts = buf->pts;
        if (this->video_first_pts == 0) {
          this->video_first_pts = buf->pts;
        }
      } else {
        /* GET */
        this->video_first_pts = buf->pts;
      }
    }

    if (video_br) {
      this->video_br = video_br;
      this->video_fifo_length_int = (8000 * this->video_fifo_size) / this->video_br;
    } else {
      if (buf->pts && (this->video_in_disc == 0)) {
        this->video_fifo_length_int = (this->video_last_pts - this->video_first_pts) / 90;
        if (this->video_fifo_length)
          this->video_br = 8000 * (this->video_fifo_size / this->video_fifo_length);
        else
          this->video_br = 0;
      } else {
        if (this->video_br)
          this->video_fifo_length_int = (8000 * this->video_fifo_size) / this->video_br;
      }
    }

  } else {
    this->audio_fifo_free = fifo_free;
    this->audio_fifo_fill = (100 * fifo_fill) / fifo_div;
    this->audio_fifo_size = fifo->fifo_data_size;

    if (buf->pts && (this->audio_in_disc == 0)) {
      if (action == FIFO_PUT) {
        this->audio_last_pts = buf->pts;
        if (!this->audio_first_pts) {
          this->audio_first_pts = buf->pts;
        }
      } else {
        /* GET */
        this->audio_first_pts = buf->pts;
      }
    }

    if (audio_br) {
      this->audio_br = audio_br;
      this->audio_fifo_length_int = (8000 * this->audio_fifo_size) / this->audio_br;
    } else {
      if (buf->pts && (this->audio_in_disc == 0)) {
        this->audio_fifo_length_int = (this->audio_last_pts - this->audio_first_pts) / 90;
        if (this->audio_fifo_length)
          this->audio_br = 8000 * (this->audio_fifo_size / this->audio_fifo_length);
        else
          this->audio_br = 0;
      } else {
        if (this->audio_br)
          this->audio_fifo_length_int = (8000 * this->audio_fifo_size) / this->audio_br;
      }
    }
  }

  /* decoder buffer compensation */
  if (has_audio && has_video) {
    diff = this->video_first_pts - this->audio_first_pts;
  } else {
    diff = 0;
  }
  if (diff > 0) {
    this->video_fifo_length = this->video_fifo_length_int + diff / 90;
    this->audio_fifo_length = this->audio_fifo_length_int;
  } else {
    this->video_fifo_length = this->video_fifo_length_int;
    this->audio_fifo_length = this->audio_fifo_length_int - diff / 90;
  }
}

/* Alloc callback */
void nbc_alloc_cb (fifo_buffer_t *fifo, void *this_gen) {
  nbc_t *this = (nbc_t*)this_gen;

  lprintf("enter nbc_alloc_cb\n");
  pthread_mutex_lock(&this->mutex);
  if (this->enabled && this->buffering) {

    /* restart playing if one fifo is full (to avoid deadlock) */
    if (fifo->buffer_pool_num_free <= 1) {
      this->progress = 100;
      report_progress (this->stream, 100);
      this->buffering = 0;

      xprintf(this->stream->xine, XINE_VERBOSITY_DEBUG, "\nnet_buf_ctrl: nbc_alloc_cb: stops buffering\n");

      nbc_set_speed_normal(this);
    }
  }
  pthread_mutex_unlock(&this->mutex);
  lprintf("exit nbc_alloc_cb\n");
}

/* Put callback
 * the fifo mutex is locked */
void nbc_put_cb (fifo_buffer_t *fifo, buf_element_t *buf, void *this_gen) {
  nbc_t *this = (nbc_t*)this_gen;
  int64_t progress = 0;
  int64_t video_p = 0;
  int64_t audio_p = 0;
  int has_video, has_audio;

  lprintf("enter nbc_put_cb\n");
  pthread_mutex_lock(&this->mutex);

  if ((buf->type & BUF_MAJOR_MASK) != BUF_CONTROL_BASE) {

    if (this->enabled) {

      nbc_compute_fifo_length(this, fifo, buf, FIFO_PUT);

      if (this->buffering) {

        has_video = _x_stream_info_get(this->stream, XINE_STREAM_INFO_HAS_VIDEO);
        has_audio = _x_stream_info_get(this->stream, XINE_STREAM_INFO_HAS_AUDIO);
        /* restart playing if high_water_mark is reached by all fifos
         * do not restart if has_video and has_audio are false to avoid
         * a yoyo effect at the beginning of the stream when these values
         * are not yet known.
         *
         * be sure that the next buffer_pool_alloc() call will not deadlock,
         * we need at least 2 buffers (see buffer.c)
         */
        if ((((!has_video) || (this->video_fifo_length > this->high_water_mark)) &&
             ((!has_audio) || (this->audio_fifo_length > this->high_water_mark)) &&
             (has_video || has_audio))) {

          this->progress = 100;
          report_progress (this->stream, 100);
          this->buffering = 0;

          xprintf(this->stream->xine, XINE_VERBOSITY_DEBUG, "\nnet_buf_ctrl: nbc_put_cb: stops buffering\n");

          nbc_set_speed_normal(this);

          this->high_water_mark += this->high_water_mark / 2;

        } else {
          /*  compute the buffering progress
           *    50%: video
           *    50%: audio */
          video_p = ((this->video_fifo_length * 50) / this->high_water_mark);
          if (video_p > 50) video_p = 50;
          audio_p = ((this->audio_fifo_length * 50) / this->high_water_mark);
          if (audio_p > 50) audio_p = 50;

          if ((has_video) && (has_audio)) {
            progress = video_p + audio_p;
          } else if (has_video) {
            progress = 2 * video_p;
          } else {
            progress = 2 * audio_p;
          }

          /* if the progress can't be computed using the fifo length,
             use the number of buffers */
          if (!progress) {
            video_p = this->video_fifo_fill;
            audio_p = this->audio_fifo_fill;
            progress = (video_p > audio_p) ? video_p : audio_p;
          }

          if (progress > this->progress) {
            report_progress (this->stream, progress);
            this->progress = progress;
          }
        }
      }
      if(this->stream->xine->verbosity >= XINE_VERBOSITY_DEBUG)
        display_stats(this);

      report_stats(this, 0);
    }
  } else {

    switch (buf->type) {
      case BUF_CONTROL_START:
        lprintf("BUF_CONTROL_START\n");
        if (!this->enabled) {
          /* a new stream starts */
          xprintf(this->stream->xine, XINE_VERBOSITY_DEBUG, "\nnet_buf_ctrl: nbc_put_cb: starts buffering\n");
          this->enabled           = 1;
          this->buffering         = 1;
          this->video_first_pts   = 0;
          this->video_last_pts    = 0;
          this->audio_first_pts   = 0;
          this->audio_last_pts    = 0;
          this->video_fifo_length = 0;
          this->audio_fifo_length = 0;
          nbc_set_speed_pause(this);
          this->progress = 0;
          report_progress (this->stream, progress);
        }
        break;
      case BUF_CONTROL_NOP:
        if (!(buf->decoder_flags & BUF_FLAG_END_USER) &&
            !(buf->decoder_flags & BUF_FLAG_END_STREAM)) {
          break;
        }
        /* fall through */
      case BUF_CONTROL_END:
      case BUF_CONTROL_QUIT:
        lprintf("BUF_CONTROL_END\n");
        if (this->enabled) {
          /* end of stream :
           *   - disable the nbc
           *   - unpause the engine if buffering
           */
          this->enabled = 0;

          lprintf("DISABLE netbuf\n");

          if (this->buffering) {
            this->buffering = 0;
            this->progress = 100;
            report_progress (this->stream, this->progress);

            xprintf(this->stream->xine, XINE_VERBOSITY_DEBUG, "\nnet_buf_ctrl: nbc_put_cb: stops buffering\n");

            nbc_set_speed_normal(this);
          }
        }
        break;

      case BUF_CONTROL_NEWPTS:
        /* discontinuity management */
        if (fifo == this->video_fifo) {
          this->video_in_disc++;
          xprintf(this->stream->xine, XINE_VERBOSITY_DEBUG,
		  "\nnet_buf_ctrl: nbc_put_cb video disc %d\n", this->video_in_disc);
        } else {
          this->audio_in_disc++;
          xprintf(this->stream->xine, XINE_VERBOSITY_DEBUG,
		  "\nnet_buf_ctrl: nbc_put_cb audio disc %d\n", this->audio_in_disc);
        }
        break;
    }

    if (fifo == this->video_fifo) {
      this->video_fifo_free = fifo->buffer_pool_num_free;
      this->video_fifo_size = fifo->fifo_data_size;
    } else {
      this->audio_fifo_free = fifo->buffer_pool_num_free;
      this->audio_fifo_size = fifo->fifo_data_size;
    }
  }
  pthread_mutex_unlock(&this->mutex);
  lprintf("exit nbc_put_cb\n");
}

/* Get callback
 * the fifo mutex is locked */
void nbc_get_cb (fifo_buffer_t *fifo,
			buf_element_t *buf, void *this_gen) {
  nbc_t *this = (nbc_t*)this_gen;

  lprintf("enter nbc_get_cb\n");
  pthread_mutex_lock(&this->mutex);

  if ((buf->type & BUF_MAJOR_MASK) != BUF_CONTROL_BASE) {

    if (this->enabled) {

      nbc_compute_fifo_length(this, fifo, buf, FIFO_GET);

      if (!this->buffering) {
        /* start buffering if one fifo is empty
         */
        int has_video = _x_stream_info_get(this->stream, XINE_STREAM_INFO_HAS_VIDEO);
        int has_audio = _x_stream_info_get(this->stream, XINE_STREAM_INFO_HAS_AUDIO);
        if (((this->video_fifo_length == 0) && has_video) ||
            ((this->audio_fifo_length == 0) && has_audio)) {
          /* do not pause if a fifo is full to avoid yoyo (play-pause-play-pause) */
          if ((this->video_fifo_free > FULL_FIFO_MARK) &&
              (this->audio_fifo_free > FULL_FIFO_MARK)) {
            this->buffering = 1;
            this->progress  = 0;
            report_progress (this->stream, 0);

            xprintf(this->stream->xine, XINE_VERBOSITY_DEBUG,
                    "\nnet_buf_ctrl: nbc_get_cb: starts buffering, vid: %d, aud: %d\n",
                    this->video_fifo_fill, this->audio_fifo_fill);
            nbc_set_speed_pause(this);
          }
        }
      } else {
        nbc_set_speed_pause(this);
      }

      if(this->stream->xine->verbosity >= XINE_VERBOSITY_DEBUG)
        display_stats(this);

      report_stats(this, 1);
    }
  } else {
    /* discontinuity management */
    if (buf->type == BUF_CONTROL_NEWPTS) {
      if (fifo == this->video_fifo) {
        this->video_in_disc--;
        xprintf(this->stream->xine, XINE_VERBOSITY_DEBUG,
		"\nnet_buf_ctrl: nbc_get_cb video disc %d\n", this->video_in_disc);
      } else {
        this->audio_in_disc--;
        xprintf(this->stream->xine, XINE_VERBOSITY_DEBUG,
		"\nnet_buf_ctrl: nbc_get_cb audio disc %d\n", this->audio_in_disc);
      }
    }

    if (fifo == this->video_fifo) {
      this->video_fifo_free = fifo->buffer_pool_num_free;
      this->video_fifo_size = fifo->fifo_data_size;
    } else {
      this->audio_fifo_free = fifo->buffer_pool_num_free;
      this->audio_fifo_size = fifo->fifo_data_size;
    }
  }

  pthread_mutex_unlock(&this->mutex);
  lprintf("exit nbc_get_cb\n");
}

nbc_t *nbc_init (xine_stream_t *stream) {

  nbc_t *this = calloc(1, sizeof (nbc_t));
  fifo_buffer_t *video_fifo = stream->video_fifo;
  fifo_buffer_t *audio_fifo = stream->audio_fifo;
  double video_fifo_factor, audio_fifo_factor;
  cfg_entry_t *entry;

  lprintf("nbc_init\n");
  pthread_mutex_init (&this->mutex, NULL);

  this->stream              = stream;
  this->video_fifo          = video_fifo;
  this->audio_fifo          = audio_fifo;

  /* when the FIFO sizes are increased compared to the default configuration,
   * apply a factor to the high water mark */
  entry = stream->xine->config->lookup_entry(stream->xine->config, "engine.buffers.video_num_buffers");
  /* No entry when no video output */
  if (entry)
    video_fifo_factor = (double)video_fifo->buffer_pool_capacity / (double)entry->num_default;
  else
    video_fifo_factor = 1.0;
  entry = stream->xine->config->lookup_entry(stream->xine->config, "engine.buffers.audio_num_buffers");
  /* When there's no audio output, there's no entry */
  if (entry)
    audio_fifo_factor = (double)audio_fifo->buffer_pool_capacity / (double)entry->num_default;
  else
    audio_fifo_factor = 1.0;
  /* use the smaller factor */
  if (video_fifo_factor < audio_fifo_factor)
    this->high_water_mark = (double)DEFAULT_HIGH_WATER_MARK * video_fifo_factor;
  else
    this->high_water_mark = (double)DEFAULT_HIGH_WATER_MARK * audio_fifo_factor;

  video_fifo->register_alloc_cb(video_fifo, nbc_alloc_cb, this);
  video_fifo->register_put_cb(video_fifo, nbc_put_cb, this);
  video_fifo->register_get_cb(video_fifo, nbc_get_cb, this);

  audio_fifo->register_alloc_cb(audio_fifo, nbc_alloc_cb, this);
  audio_fifo->register_put_cb(audio_fifo, nbc_put_cb, this);
  audio_fifo->register_get_cb(audio_fifo, nbc_get_cb, this);

  return this;
}

void nbc_close (nbc_t *this) {
  fifo_buffer_t *video_fifo = this->stream->video_fifo;
  fifo_buffer_t *audio_fifo = this->stream->audio_fifo;
  xine_t        *xine       = this->stream->xine;

  xprintf(xine, XINE_VERBOSITY_DEBUG, "\nnet_buf_ctrl: nbc_close\n");

  /* unregister all fifo callbacks */
  /* do not lock the mutex to avoid deadlocks if a decoder calls fifo->get() */
  video_fifo->unregister_alloc_cb(video_fifo, nbc_alloc_cb);
  video_fifo->unregister_put_cb(video_fifo, nbc_put_cb);
  video_fifo->unregister_get_cb(video_fifo, nbc_get_cb);

  audio_fifo->unregister_alloc_cb(audio_fifo, nbc_alloc_cb);
  audio_fifo->unregister_put_cb(audio_fifo, nbc_put_cb);
  audio_fifo->unregister_get_cb(audio_fifo, nbc_get_cb);

  /* now we are sure that nobody will call a callback */
  this->stream->xine->clock->set_option (this->stream->xine->clock, CLOCK_SCR_ADJUSTABLE, 1);

  pthread_mutex_destroy(&this->mutex);
  free (this);
  xprintf(xine, XINE_VERBOSITY_DEBUG, "\nnet_buf_ctrl: nbc_close: done\n");
}
