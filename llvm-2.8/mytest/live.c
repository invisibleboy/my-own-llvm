int live()
{
	int u,v,w,x,y,z;
	v = 1;
        z = v + 1;
	x = z * v;
	y = x * 2;
 	w = x + z * y;
	u = z + 2;
	v = u + w + y;
	return v * u;
}
