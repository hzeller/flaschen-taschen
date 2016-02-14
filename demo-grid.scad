// Little grid to put on top of the LED matrix so that it immitates the
// look of milk crate.

$fn=12;
epsilon = 0.01;

// Printer compensation: My printer is not entirely square :/
M = [ [ 1, 0, 0, 0 ],
      [ -2/200, 1, 0, 0 ],  // compensate for 2 mm off per 200mm
      [ 0, 0, 1, 0 ],
      [ 0, 0, 0, 1 ]
  ] ;
   
module bottles() {
    translate([-25/2,-25/2]) {
	r=3.8/2;
	for (x = [0:4]) {
	    for (y = [0:4]) {
		%translate([x*5+5/2,y*5+5/2]) cylinder(r=r, h=1);
	    }
	}
    }
}

module rounded_flat(r, h, d) {
    translate([-d/2, -d/2]) hull() {
	translate([r,r]) cylinder(r=r, h=h);
	translate([r,d-r]) cylinder(r=r, h=h);
	translate([d-r,d-r]) cylinder(r=r, h=h);
	translate([d-r,r]) cylinder(r=r, h=h);
    }
}

module crate(frame_thick=1.2) {
    difference() {
	rounded_flat(5/2, frame_thick, 25.1); // little overlap.
	translate([0,0,-epsilon]) rounded_flat(4/2, frame_thick+2*epsilon, 24.2);
    }
    intersection() {
	union() {
	    rotate([0,0,45]) grid();
	    rotate([0,0,-45]) grid();
	}
	rounded_flat(5/2, frame_thick, 25);
    }
}

module FlaschenTaschen(frame_thick=1) {
    for (x = [0:9-1]) {
	for (y = [0:7-1]) {
	    translate([x*25,y*25,0]) crate(frame_thick=frame_thick);
	}
    }
}

module grid(w=50,h=50,thick=0.6, barw=0.4, distance=4.3) {
    translate([0,-h/2,0]) {
	for ( d = [-10*distance:distance:10*distance]) {
	    translate([d - barw/2, 0, 0]) cube([barw, h, thick]);
	}
    }
}

//crate(frame_thick=2);
bottles();

multmatrix(M) FlaschenTaschen(frame_thick=2);