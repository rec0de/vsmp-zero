vsmp: vsmp.c vsmp.h IT8951.c IT8951.h
	gcc -o vsmp vsmp.c IT8951.c -O2 -L/opt/vc/lib -lbcm2835 -latomic `pkg-config --cflags --libs libavformat libavcodec libavutil`

debug: vsmp.c vsmp.h
	gcc -o vsmp vsmp.c -O2 -lavutil -lavcodec -lavformat
