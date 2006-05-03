set terminal fig
plot [0:4*pi] [-1:1] (x<pi || (x>2*pi && x<3*pi))?sin(x)*abs(sin(x)):0
