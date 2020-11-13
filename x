
[0.2, 0.5, 1.0, 1]



where z_near = 0.1;
where z_far  = 100.0;

after transformation:
    if z == 0.1, then pz = -1.0;
    if z == 100.0, then pz = 1.0;

therefore:

z = 0.1,   z / -z_near
z = 100.0, z / z_far

