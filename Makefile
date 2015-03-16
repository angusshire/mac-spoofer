default: run

run: mac-spoofer.exe
	./mac-spoofer

mac-spoofer.exe: mac-spoofer.cpp
	g++ -g -o $@ $< -lole32 -lWbemuuid -loleAut32

clean:
	rm mac-spoofer.exe
