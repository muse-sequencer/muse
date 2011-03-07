set terminal fig
plot [0:4*pi] [-1:1] (x<pi || (x>2*pi && x<3*pi))?sin(2*x)*abs(sin(2*x)):0
