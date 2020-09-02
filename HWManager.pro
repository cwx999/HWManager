TEMPLATE = subdirs
SUBDIRS += \
    LibHWDio\
    HWDemo \
    ConsoleTest \
    LibHWAio \
    LibHWAudio
HWDemo.depends = LibHWAio
HWDemo.depends = LibHWDio
HWDemo.depends = LibHWAudio
ConsoleTest.depends = LibHWDio

