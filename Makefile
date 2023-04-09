all:
	gcc -o pingpong.exe ppos-core-aux.c pingpong-scheduler.c libppos_static.a