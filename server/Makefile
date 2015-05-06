motor_server: motor_server.o
	cc -o $@ $^ -lwiringPi

.c.o:
	cc -c $<

clean:
	@rm motor_server
	@rm motor_server.o

# vim: noexpandtab
