#CC := g++-4.0
targets := cube hypercube rcube rhypercube
CXXFLAGS := `sdl-config --cflags` -O3 -Wall -g
LDFLAGS := `sdl-config --libs`

.PHONY:	all
all:	$(targets)

hypercube: hypercube.cc

rhypercube: rhypercube.cc

cube: cube.cc

rcube: rcube.cc

.PHONY:	clean
clean:
	@-$(RM) $(targets)

