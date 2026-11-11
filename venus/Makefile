TOP_DIR:=$(TOP_DIR)
ROOT_DIR:=$(TOP_DIR)/../..
MAKEFILE_PATH:=$(subst $(strip \),/,$(abspath $(lastword $(MAKEFILE_LIST))))

SRC_FILES:= \
  $(filter-out app/src/vn_celluar_app.c,$(wildcard app/src/*.c)) \
  $(wildcard bsp/*/*.c) \
  $(wildcard component/*/*.c) \
  $(wildcard component/vn_ota/*/*.c) \
  $(wildcard xiaozhi/src/*.c) \
  $(wildcard resource/*.c) \
  $(wildcard resource/*/*.c) \
  $(wildcard third_party/at_command/src/*.c) \
  $(wildcard third_party/EasyFlash/src/*.c) \
  $(wildcard third_party/mongoose/*.c)

INC_DIRS+= \
  -I${TOP_DIR}/venus/third_party/mongoose \
  -I${TOP_DIR}/ \
  -I${TOP_DIR}/common \
  -I${TOP_DIR}/common/include \
  -I${TOP_DIR}/common/include/lwipv4v6 \
  -I${TOP_DIR}/../../ql-kernel/threadx/common/include \
  -I${TOP_DIR}/venus \
  -I${TOP_DIR}/venus/third_party/at_command/inc \
  -I${TOP_DIR}/venus/third_party/EasyFlash/inc \
  -I${TOP_DIR}/venus/third_party/vn_opus/inc \
  -I${TOP_DIR}/venus/third_party/lvgl \
  -I${TOP_DIR}/venus/third_party/lvgl/src \
  -I${TOP_DIR}/venus/third_party/lvgl/src/extra \
  $(addprefix -I${TOP_DIR}/venus/,$(wildcard app/inc bsp/* component/* component/vn_ota/* xiaozhi/inc xiaozhi/src) resource/gif resource/emoji resource/icon)

DFLAGS+= \
  -D__TM_ZONE=tm_zone \
  -D__TM_GMTOFF=tm_gmtoff \
  -D_POSIX_THREADS \
  -D_UNIX98_THREAD_MUTEX_ATTRIBUTES \
  -DLV_CONF_INCLUDE_SIMPLE

U_FLAGS:=
U_CPPFLAGS:=
U_LIBS:=

include ${TOP_DIR}/config/common/makefile.mk
