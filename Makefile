default: run

run: changeMACAddress.exe
	./changeMACAddress

changeMACAddress.exe: changeMACAddress.cpp
	g++ -g -o $@ $< -lole32 -lWbemuuid -loleAut32

clean:
	rm changeMACAddress.exe
