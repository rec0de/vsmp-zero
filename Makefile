vsmp: vsmp.c vsmp.h dither.c displays/*
	gcc -o vsmp vsmp.c -O2 -L/opt/vc/lib -lbcm2835 -latomic `pkg-config --cflags --libs libavformat libavcodec libavutil`

debug: vsmp.c vsmp.h dither.c displays/dryrun.c
	gcc -o vsmp vsmp.c -O2 -lavutil -lavcodec -lavformat
