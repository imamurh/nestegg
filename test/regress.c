/*
 * Copyright © 2014 Mozilla Foundation
 *
 * This program is made available under an ISC-style license.  See the
 * accompanying file LICENSE for details.
 */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "nestegg/nestegg.h"

#include "sha1.c"

static int
stdio_read(void * p, size_t length, void * file)
{
  size_t r;
  FILE * fp = file;

  r = fread(p, length, 1, fp);
  if (r == 0 && feof(fp))
    return 0;
  return r == 0 ? -1 : 1;
}

static int
stdio_seek(int64_t offset, int whence, void * file)
{
  FILE * fp = file;
  return fseek(fp, offset, whence);
}

static int64_t
stdio_tell(void * fp)
{
  return ftell(fp);
}

int
test(char const * path)
{
  FILE * fp;
  int r, type;
  nestegg * ctx;
  nestegg_audio_params aparams;
  nestegg_packet * pkt;
  nestegg_video_params vparams;
  size_t length, size;
  uint64_t duration, pkt_tstamp;
  unsigned char * codec_data, * ptr;
  unsigned int i, j, tracks, pkt_cnt, pkt_track;
  unsigned int data_items = 0;
  nestegg_io io = {
    stdio_read,
    stdio_seek,
    stdio_tell,
    NULL
  };

  fp = fopen(path, "rb");
  if (!fp)
    return EXIT_FAILURE;

  io.userdata = fp;

  ctx = NULL;
  r = nestegg_init(&ctx, io, NULL, -1);
  if (r != 0)
    return EXIT_FAILURE;

  nestegg_track_count(ctx, &tracks);
  nestegg_duration(ctx, &duration);

  for (i = 0; i < tracks; ++i) {
    type = nestegg_track_type(ctx, i);
    nestegg_track_codec_data_count(ctx, i, &data_items);
    for (j = 0; j < data_items; ++j) {
      nestegg_track_codec_data(ctx, i, j, &codec_data, &length);
    }
    if (type == NESTEGG_TRACK_VIDEO) {
      nestegg_track_video_params(ctx, i, &vparams);
    } else if (type == NESTEGG_TRACK_AUDIO) {
      nestegg_track_audio_params(ctx, i, &aparams);
    }
  }

  while (nestegg_read_packet(ctx, &pkt) > 0) {
    nestegg_packet_track(pkt, &pkt_track);
    nestegg_packet_count(pkt, &pkt_cnt);
    nestegg_packet_tstamp(pkt, &pkt_tstamp);

    printf("%u %llu %u", pkt_track, (unsigned long long) pkt_tstamp, pkt_cnt);

    for (i = 0; i < pkt_cnt; ++i) {
      nestegg_packet_data(pkt, i, &ptr, &size);
      printf(" %u", (unsigned int) size);
    }
    printf("\n");

    nestegg_free_packet(pkt);
  }

  nestegg_destroy(ctx);
  fclose(fp);
  return EXIT_SUCCESS;
}

int
main(int argc, char * argv[])
{
  if (argc != 2)
    return EXIT_FAILURE;

  return test(argv[1]);
}
