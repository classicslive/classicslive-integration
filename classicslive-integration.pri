# Paths
CL_DIR = $$PWD
CL_EDITOR_DIR = $$CL_DIR/editor
CL_LIBRETRO_DIR = $$PWD/../libretro-common

# Include Paths
INCLUDEPATH += \
    $$CL_DIR \
    $$CL_EDITOR_DIR \
    $$CL_LIBRETRO_DIR/include

# Classics Live Sources
SOURCES += \
    $$CL_DIR/cl_abi.c \
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
    $$CL_DIR/cl_search.c \
    $$CL_DIR/3rdparty/jsonsax/jsonsax.c \
    $$CL_DIR/3rdparty/jsonsax/jsonsax_full.c

HEADERS += \
    $$CL_DIR/cl_abi.h \
    $$CL_DIR/cl_action.h \
    $$CL_DIR/cl_common.h \
    $$CL_DIR/cl_config.h \
    $$CL_DIR/cl_counter.h \
    $$CL_DIR/cl_identify.h \
    $$CL_DIR/cl_json.h \
    $$CL_DIR/cl_main.h \
    $$CL_DIR/cl_memory.h \
    $$CL_DIR/cl_network.h \
    $$CL_DIR/cl_script.h \
    $$CL_DIR/cl_search.h \
    $$CL_DIR/cl_types.h \
    $$CL_DIR/3rdparty/jsonsax/jsonsax.h \
    $$CL_DIR/3rdparty/jsonsax/jsonsax_full.h

# Live Editor sources
contains(DEFINES, CL_HAVE_EDITOR=1) {
    SOURCES += \
        $$CL_EDITOR_DIR/cle_action_block.cpp \
        $$CL_EDITOR_DIR/cle_action_block_api.cpp \
        $$CL_EDITOR_DIR/cle_action_block_bookend.cpp \
        $$CL_EDITOR_DIR/cle_action_block_comparison.cpp \
        $$CL_EDITOR_DIR/cle_action_block_ctrbinary.cpp \
        $$CL_EDITOR_DIR/cle_action_block_ctrunary.cpp \
        $$CL_EDITOR_DIR/cle_action_block_nop.cpp \
        $$CL_EDITOR_DIR/cle_common.cpp \
        $$CL_EDITOR_DIR/cle_hex_view.cpp \
        $$CL_EDITOR_DIR/cle_main.cpp \
        $$CL_EDITOR_DIR/cle_memory_inspector.cpp \
        $$CL_EDITOR_DIR/cle_memory_note_submit.cpp \
        $$CL_EDITOR_DIR/cle_memory_notes.cpp \
        $$CL_EDITOR_DIR/cle_pointer_search_dialog.cpp \
        $$CL_EDITOR_DIR/cle_result_table.cpp \
        $$CL_EDITOR_DIR/cle_result_table_normal.cpp \
        $$CL_EDITOR_DIR/cle_result_table_pointer.cpp \
        $$CL_EDITOR_DIR/cle_script_editor_block.cpp \
        $$CL_EDITOR_DIR/cle_script_editor_block_canvas.cpp

    HEADERS += \
        $$CL_EDITOR_DIR/cle_action_block.h \
        $$CL_EDITOR_DIR/cle_action_block_api.h \
        $$CL_EDITOR_DIR/cle_action_block_bookend.h \
        $$CL_EDITOR_DIR/cle_action_block_comparison.h \
        $$CL_EDITOR_DIR/cle_action_block_ctrbinary.h \
        $$CL_EDITOR_DIR/cle_action_block_ctrunary.h \
        $$CL_EDITOR_DIR/cle_action_block_nop.h \
        $$CL_EDITOR_DIR/cle_common.h \
        $$CL_EDITOR_DIR/cle_hex_view.h \
        $$CL_EDITOR_DIR/cle_main.h \
        $$CL_EDITOR_DIR/cle_memory_inspector.h \
        $$CL_EDITOR_DIR/cle_memory_notes.h \
        $$CL_EDITOR_DIR/cle_memory_note_submit.h \
        $$CL_EDITOR_DIR/cle_pointer_search_dialog.h \
        $$CL_EDITOR_DIR/cle_result_table.h \
        $$CL_EDITOR_DIR/cle_result_table_normal.h \
        $$CL_EDITOR_DIR/cle_result_table_pointer.h \
        $$CL_EDITOR_DIR/cle_script_editor_block.h \
        $$CL_EDITOR_DIR/cle_script_editor_block_canvas.h
}

# libretro-common files
SOURCES += \
    $$CL_LIBRETRO_DIR/compat/compat_strl.c \
    $$CL_LIBRETRO_DIR/compat/fopen_utf8.c \
    $$CL_LIBRETRO_DIR/encodings/encoding_base64.c \
    $$CL_LIBRETRO_DIR/encodings/encoding_crc32.c \
    $$CL_LIBRETRO_DIR/encodings/encoding_utf.c \
    $$CL_LIBRETRO_DIR/file/file_path.c \
    $$CL_LIBRETRO_DIR/file/file_path_io.c \
    $$CL_LIBRETRO_DIR/formats/libchdr/libchdr_bitstream.c \
    $$CL_LIBRETRO_DIR/formats/libchdr/libchdr_cdrom.c \
    $$CL_LIBRETRO_DIR/formats/libchdr/libchdr_chd.c \
    $$CL_LIBRETRO_DIR/formats/libchdr/libchdr_huffman.c \
    $$CL_LIBRETRO_DIR/formats/libchdr/libchdr_zlib.c \
    $$CL_LIBRETRO_DIR/hash/lrc_hash.c \
    $$CL_LIBRETRO_DIR/streams/chd_stream.c \
    $$CL_LIBRETRO_DIR/streams/file_stream.c \
    $$CL_LIBRETRO_DIR/streams/interface_stream.c \
    $$CL_LIBRETRO_DIR/streams/memory_stream.c \
    $$CL_LIBRETRO_DIR/streams/rzip_stream.c \
    $$CL_LIBRETRO_DIR/streams/trans_stream.c \
    $$CL_LIBRETRO_DIR/streams/trans_stream_pipe.c \
    $$CL_LIBRETRO_DIR/streams/trans_stream_zlib.c \
    $$CL_LIBRETRO_DIR/string/stdstring.c \
    $$CL_LIBRETRO_DIR/time/rtime.c \
    $$CL_LIBRETRO_DIR/utils/md5.c \
    $$CL_LIBRETRO_DIR/vfs/vfs_implementation.c

DEFINES += HAVE_ZLIB=1

LIBS += -lz
