vsmp: vsmp.c IT8951.c
	gcc -o vsmp vsmp.c IT8951.c -O2 -L/opt/vc/lib -lbcm2835 -latomic `pkg-config --cflags --libs libavformat libavcodec libavutil`

debug: vsmp.c
	gcc -o vsmp vsmp.c -O2 -lavutil -lavcodec -lavformat