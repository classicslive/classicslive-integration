# Paths
CL_DIR = $$PWD
CL_LIBRETRO_DIR = $$PWD/../libretro-common

# Include Paths
INCLUDEPATH += \
    $$CL_DIR \
    $$CL_LIBRETRO_DIR/include

# Classics Live Sources
SOURCES += \
    $$CL_DIR/cl_action.c \
    $$CL_DIR/cl_common.c \
    $$CL_DIR/cl_config.c \
    $$CL_DIR/cl_counter.c \
    $$CL_DIR/cl_identify.c \
    $$CL_DIR/cl_json.c \
    $$CL_DIR/cl_main.c \
    $$CL_DIR/cl_memory.c \
    $$CL_DIR/cl_network.c \
    $$CL_DIR/cl_script.c \
    $$CL_DIR/cl_search.c

HEADERS += \
    $$CL_DIR/cl_action.h \
    $$CL_DIR/cl_common.h \
    $$CL_DIR/cl_config.h \
    $$CL_DIR/cl_counter.h \
    $$CL_DIR/cl_frontend.h \
    $$CL_DIR/cl_identify.h \
    $$CL_DIR/cl_json.h \
    $$CL_DIR/cl_main.h \
    $$CL_DIR/cl_memory.h \
    $$CL_DIR/cl_network.h \
    $$CL_DIR/cl_script.h \
    $$CL_DIR/cl_search.h \
    $$CL_DIR/cl_types.h

# libretro-common files
SOURCES += \
    $$CL_LIBRETRO_DIR/compat/compat_strl.c \
    $$CL_LIBRETRO_DIR/compat/fopen_utf8.c \
    $$CL_LIBRETRO_DIR/encodings/encoding_base64.c \
    $$CL_LIBRETRO_DIR/encodings/encoding_crc32.c \
    $$CL_LIBRETRO_DIR/encodings/encoding_utf.c \
    $$CL_LIBRETRO_DIR/file/file_path.c \
    $$CL_LIBRETRO_DIR/formats/json/jsonsax.c \
    $$CL_LIBRETRO_DIR/formats/json/jsonsax_full.c \
    $$CL_LIBRETRO_DIR/formats/libchdr/libchdr_bitstream.c \
    $$CL_LIBRETRO_DIR/formats/libchdr/libchdr_cdrom.c \
    $$CL_LIBRETRO_DIR/formats/libchdr/libchdr_chd.c \
    $$CL_LIBRETRO_DIR/formats/libchdr/libchdr_huffman.c \
    $$CL_LIBRETRO_DIR/hash/lrc_hash.c \
    $$CL_LIBRETRO_DIR/streams/chd_stream.c \
    $$CL_LIBRETRO_DIR/streams/file_stream.c \
    $$CL_LIBRETRO_DIR/streams/interface_stream.c \
    $$CL_LIBRETRO_DIR/streams/memory_stream.c \
    $$CL_LIBRETRO_DIR/string/stdstring.c \
    $$CL_LIBRETRO_DIR/time/rtime.c \
    $$CL_LIBRETRO_DIR/utils/md5.c \
    $$CL_LIBRETRO_DIR/vfs/vfs_implementation.c
