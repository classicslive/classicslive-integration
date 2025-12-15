# Paths
CLASSICS_LIVE_DIR ?= $(PWD)
CLASSICS_LIVE_LIBRETRO_DIR ?= $(PWD)/../libretro-common

CLASSICS_LIVE_SOURCES_CLASSICSLIVE = \
    $(CLASSICS_LIVE_DIR)/cl_abi.c \
    $(CLASSICS_LIVE_DIR)/cl_action.c \
    $(CLASSICS_LIVE_DIR)/cl_common.c \
    $(CLASSICS_LIVE_DIR)/cl_config.c \
    $(CLASSICS_LIVE_DIR)/cl_counter.c \
    $(CLASSICS_LIVE_DIR)/cl_identify.c \
    $(CLASSICS_LIVE_DIR)/cl_json.c \
    $(CLASSICS_LIVE_DIR)/cl_main.c \
    $(CLASSICS_LIVE_DIR)/cl_memory.c \
    $(CLASSICS_LIVE_DIR)/cl_network.c \
    $(CLASSICS_LIVE_DIR)/cl_script.c \
    $(CLASSICS_LIVE_DIR)/cl_search.c \
    $(CLASSICS_LIVE_DIR)/3rdparty/jsonsax/jsonsax.c \
    $(CLASSICS_LIVE_DIR)/3rdparty/jsonsax/jsonsax_full.c \

CLASSICS_LIVE_SOURCES_LIBRETRO = \
    $(CLASSICS_LIVE_LIBRETRO_DIR)/compat/compat_strl.c \
    $(CLASSICS_LIVE_LIBRETRO_DIR)/compat/fopen_utf8.c \
    $(CLASSICS_LIVE_LIBRETRO_DIR)/encodings/encoding_base64.c \
    $(CLASSICS_LIVE_LIBRETRO_DIR)/encodings/encoding_crc32.c \
    $(CLASSICS_LIVE_LIBRETRO_DIR)/encodings/encoding_utf.c \
    $(CLASSICS_LIVE_LIBRETRO_DIR)/file/file_path.c \
    $(CLASSICS_LIVE_LIBRETRO_DIR)/file/file_path_io.c \
    $(CLASSICS_LIVE_LIBRETRO_DIR)/hash/lrc_hash.c \
    $(CLASSICS_LIVE_LIBRETRO_DIR)/streams/file_stream.c \
    $(CLASSICS_LIVE_LIBRETRO_DIR)/streams/memory_stream.c \
    $(CLASSICS_LIVE_LIBRETRO_DIR)/string/stdstring.c \
    $(CLASSICS_LIVE_LIBRETRO_DIR)/time/rtime.c \
    $(CLASSICS_LIVE_LIBRETRO_DIR)/utils/md5.c \
    $(CLASSICS_LIVE_LIBRETRO_DIR)/vfs/vfs_implementation.c

ifeq ($(CL_HAVE_FILESYSTEM),1)
CLASSICS_LIVE_SOURCES_LIBRETRO += \
    $(CLASSICS_LIVE_LIBRETRO_DIR)/streams/chd_stream.c \
    $(CLASSICS_LIVE_LIBRETRO_DIR)/formats/libchdr/libchdr_bitstream.c \
    $(CLASSICS_LIVE_LIBRETRO_DIR)/formats/libchdr/libchdr_cdrom.c \
    $(CLASSICS_LIVE_LIBRETRO_DIR)/formats/libchdr/libchdr_chd.c \
    $(CLASSICS_LIVE_LIBRETRO_DIR)/formats/libchdr/libchdr_huffman.c \
    $(CLASSICS_LIVE_LIBRETRO_DIR)/formats/libchdr/libchdr_zlib.c \
    $(CLASSICS_LIVE_LIBRETRO_DIR)/streams/interface_stream.c \
    $(CLASSICS_LIVE_LIBRETRO_DIR)/streams/rzip_stream.c \
    $(CLASSICS_LIVE_LIBRETRO_DIR)/streams/trans_stream.c \
    $(CLASSICS_LIVE_LIBRETRO_DIR)/streams/trans_stream_pipe.c \
    $(CLASSICS_LIVE_LIBRETRO_DIR)/streams/trans_stream_zlib.c

CLASSICS_LIVE_CFLAGS += -DHAVE_ZLIB=1
CLASSICS_LIVE_LIBS += -lz
endif

CLASSICS_LIVE_SOURCES = \
    $(CLASSICS_LIVE_SOURCES_CLASSICSLIVE) \
    $(CLASSICS_LIVE_SOURCES_LIBRETRO)
