all: write_app read_app ioctl_app

write_app: write_app.c
	$(CC) write_app.c -o write_app

read_app: read_app.c
	$(CC) read_app.c -o read_app

ioctl_app: ioctl_app.c
	$(CC) ioctl_app.c -o ioctl_app

clean:
	rm -f write_app read_app ioctl_app
