include <triangles.scad>;

// Dimensions are in mm

// Fragment count. Use this many fragments to make a circle.
$fn = 360;

base_length = 180;
base_width = 180;
base_thickness = 5;
// cover holder height - this is the RPI height plus the 10mm standoffs, plus a bit
base_height = 60;

// thickness of lexan cover
lexan_thickness = 4;

// thickness of "stands" to support the above lexan cover
cover_support_thickness = 3;

// how much the angle should come out into the main base
cover_support_reinforcement = 15;

// all our standoffs are the same size
standoff_height = 10;
standoff_diameter = 5;

// screws. The hole should be large enough to not engage the threads.
screw_hole_diameter = 3;
screw_head_diameter = 5;
screw_hole_height = 1000; // more or less infinitely high.
// min thickness of the board for the screws to actually be 
// held by
board_screw_thickness = 1;

// larger screws for power converter. These are coarse thread
// self-tapping screws that are just screwed into the backer.
// This should be small enough to enage the threads,
large_screw_hole_diameter = 3.5;

// extra standoffs for the converter, because the screws I'm
// using are 13mm long and 3mm wings and a 5mm base means they would
// stick out the back. So make a 10mm standoff, measured from the bottom
// of the base (not elevated) to screw the screws into
large_screw_standoff_height = 10;

// anchor screws for mounting to the wall. This should be
// large enough to not engage the threads.
anchor_screw_hole_diameter = 6;

// SSR board volume with standoffs
module makeSSRBoard() {
    length = 73;
    width = 62;
    height = 25;
    
    union() {
        // base board volume
        cube([length, width, height]);
        
        // standoffs
        translate([5, 5, -standoff_height]) {
            cylinder(d = standoff_diameter, h = standoff_height);
        }
        
        translate([5, 56, -standoff_height]) {
            cylinder(d = standoff_diameter, h = standoff_height);
        }
        
        translate([67, 5, -standoff_height]) {
            cylinder(d = standoff_diameter, h = standoff_height);
        }
        
        translate([67, 56, -standoff_height]) {
            cylinder(d = standoff_diameter, h = standoff_height);
        }
    }
}

// this is a positive add that needs to be differenced out by whatever calls it.
module makeSSRBoardHoles() {
    // shoot some holes in it for the screws - they are infinitely tall so we can see them at the top.

    // shafts
    translate([5, 5, -screw_hole_height/2]) {
        cylinder(d = screw_hole_diameter, h = screw_hole_height);
    }
        
    translate([5, 56, -screw_hole_height/2]) {
        cylinder(d = screw_hole_diameter, h = screw_hole_height);
    }
        
    translate([67, 5, -screw_hole_height/2]) {
        cylinder(d = screw_hole_diameter, h = screw_hole_height);
    }
     
    translate([67, 56, -screw_hole_height/2]) {
        cylinder(d = screw_hole_diameter, h = screw_hole_height);
    }
    
    // heads
    translate([5, 5, -(base_thickness + board_screw_thickness + standoff_height)]) {
        cylinder(d = screw_head_diameter, h = base_thickness);
    }
    
    translate([5, 56, -(base_thickness + board_screw_thickness + standoff_height)]) {
        cylinder(d = screw_head_diameter, h = base_thickness);
    }
    
    translate([67, 5, -(base_thickness + board_screw_thickness + standoff_height)]) {
        cylinder(d = screw_head_diameter, h = base_thickness);
    }
    
    translate([67, 56, -(base_thickness + board_screw_thickness + standoff_height)]) {
        cylinder(d = screw_head_diameter, h = base_thickness);
    }
}

module makeRPI() {
    length = 88;
    width = 60;
    height = 41; // includes 15mm of space to plug into header pins and route wires
         
    union() {
        // base board volume
        cube([length, width, height]);
        
        // standoffs
        translate([3.5, 6.5, -standoff_height]) {
            cylinder(d = standoff_diameter, h = standoff_height);
        }

        translate([3.5, 55.5, -standoff_height]) {
            cylinder(d = standoff_diameter, h = standoff_height);
        }

        translate([61.5, 6.5, -standoff_height]) {
            cylinder(d = standoff_diameter, h = standoff_height);
        }

        translate([61.5, 55.5, -standoff_height]) {
            cylinder(d = standoff_diameter, h = standoff_height);
        }
    }
}

// this is a positive add that needs to be differenced out by whatever calls it.
module makeRPIBoardHoles() {
    // shoot some holes in it for the screws - they are infinitely tall so we can see them at the top.

    // shafts
    translate([3.5, 6.5, -screw_hole_height/2]) {
        cylinder(d = screw_hole_diameter, h = screw_hole_height);
    }
        
    translate([3.5, 55.5, -screw_hole_height/2]) {
        cylinder(d = screw_hole_diameter, h = screw_hole_height);
    }
        
    translate([61.5, 6.5, -screw_hole_height/2]) {
        cylinder(d = screw_hole_diameter, h = screw_hole_height);
    }
     
    translate([61.5, 55.5, -screw_hole_height/2]) {
        cylinder(d = screw_hole_diameter, h = screw_hole_height);
    }
    
    // heads
    translate([3.5, 6.5, -(base_thickness + board_screw_thickness + standoff_height)]) {
        cylinder(d = screw_head_diameter, h = base_thickness);
    }
    
    translate([3.5, 55.5, -(base_thickness + board_screw_thickness + standoff_height)]) {
        cylinder(d = screw_head_diameter, h = base_thickness);
    }
    
    translate([61.5, 6.5, -(base_thickness + board_screw_thickness + standoff_height)]) {
        cylinder(d = screw_head_diameter, h = base_thickness);
    }
    
    translate([61.5, 55.5, -(base_thickness + board_screw_thickness + standoff_height)]) {
        cylinder(d = screw_head_diameter, h = base_thickness);
    }
}

module makeConverter() {
    length = 46.5;
    width = 31.5;
    height = 18;
      
    // no standoffs, but it has "wings" with U cutouts
    // for the screws
    
    wing_width = 13;
    wing_length = 9;
    wing_height = 3.5;
    
    cutout_width = 4;
    cutout_length = 7;
    
    union() {
        // base board volume
        cube([length, width, height]);
        
        // wings
        translate([length, width/2 - wing_width/2, 0]) {
            difference() {
                cube([wing_length, wing_width, wing_height]);
                translate([wing_length - cutout_length + 0.1,
                           wing_width/2 - cutout_width/2,
                           -wing_height/2]) {
                    cube([cutout_length, cutout_width, wing_height * 2]);
                }
            }
        }
        
        translate([-wing_length, width/2 - wing_width/2, 0]) {
            difference() {
                cube([wing_length, wing_width, wing_height]);
                translate([-(wing_length - cutout_length - 1.9),
                           wing_width/2 - cutout_width/2,
                           -wing_height/2]) {
                    cube([cutout_length, cutout_width, wing_height * 2]);
                }
            }
        }
    }
}

// this is a positive add that needs to be differenced out by whatever calls it.
module makeConverterBoardHoles() {
    
    // shafts
    translate([-5, 15.75, -screw_hole_height/2]) {
        cylinder(d = large_screw_hole_diameter, h = screw_hole_height);
    }
        
    translate([52, 15.75, -screw_hole_height/2]) {
        cylinder(d = large_screw_hole_diameter, h = screw_hole_height);
    }
}

module make18B20Board() {
    length = 24.5;
    width = 21;
    height = 14.5; // includes 15mm of space to plug into header pins and route wires
    
    union() {
        // base board volume
        cube([length, width, height]);
        
        // standoffs
        translate([2.5, 2.5, -standoff_height]) {
            cylinder(d = standoff_diameter, h = base_thickness);
        }

        translate([2.5, 18, -standoff_height]) {
            cylinder(d = standoff_diameter, h = base_thickness);
        }
    }
    
}

// this is a positive add that needs to be differenced out by whatever calls it.
module make18B20BoardHoles() {
    
    // shafts
    translate([2.5, 2.5, -screw_hole_height/2]) {
        cylinder(d = screw_hole_diameter, h = screw_hole_height);
    }
        
    translate([2.5, 18, -screw_hole_height/2]) {
        cylinder(d = screw_hole_diameter, h = screw_hole_height);
    }
    
    // heads
    translate([2.5, 2.5, -(base_thickness + board_screw_thickness + standoff_height)]) {
        cylinder(d = screw_head_diameter, h = base_thickness);
    }
    
    translate([2.5, 18, -(base_thickness + board_screw_thickness + standoff_height)]) {
        cylinder(d = screw_head_diameter, h = base_thickness);
    }
}

// A cover holder of a given width and a predefined thickness
// and height. It also adds a groove for the lexan, supports
// and the ends, and another in the middle if the width is more
// than 50mm, if center_support = true
module makeCoverHolder(width, center_support = true) {
    
    // the vertical part
    cube([cover_support_thickness, width, base_height+base_thickness]);
    
    // supports for the vertical part
    translate([cover_support_thickness,
               cover_support_thickness/2,
               base_thickness]) {
            Wedge(a=base_height * .75,
                  b=cover_support_reinforcement,
                  w1 = cover_support_thickness,
                  w2 = cover_support_thickness);
    }
    
    translate([cover_support_thickness,
               width - cover_support_thickness/2,
               base_thickness]) {
            Wedge(a=base_height * .75,
                  b=cover_support_reinforcement,
                  w1 = cover_support_thickness,
                  w2 = cover_support_thickness);
    }
    
    if (width > 50 && center_support) {
        translate([cover_support_thickness,
                   width/2 - cover_support_thickness/2,
                   base_thickness]) {
                Wedge(a=base_height * .75,
                      b=cover_support_reinforcement,
                      w1 = cover_support_thickness,
                      w2 = cover_support_thickness);
        }
    }
    
    // upper part of groove for lexan - angled to allow it to print
    translate([cover_support_thickness,
               width,
               base_height + base_thickness]) {
        rotate([90,90,0]) {
            Right_Angled_Triangle(a=cover_support_thickness,
                                  b=cover_support_thickness,
                                  height=width,
                                  center=false);
        }
    }
    
    // lower part of groove for lexan - angled to allow it to print
    translate([cover_support_thickness,
               width,
               base_height + base_thickness - lexan_thickness - cover_support_thickness]) {
        rotate([90,90,0]) {
            Right_Angled_Triangle(a=cover_support_thickness,
                                  b=cover_support_thickness,
                                  height=width,
                                  center=false);
        }
    }
}

module makeBase() {
    cube([base_length, base_width, base_thickness]);
    
    // "clips" to hold lexan
    makeCoverHolder(65);
    
    // the extra standoffs
    translate([115.1, 85.6, 0]) {
        cylinder(d=10, h=large_screw_standoff_height);
        translate([57, 0, 0]) {
            cylinder(d=10, h=large_screw_standoff_height);
        }
    }

    // these next two end up squished together to make a corner, so 
    // I made the union explicit to remind myself...
    union() {
        translate([0, base_width - 35, 0]) {
            makeCoverHolder(35);
        }
        
        translate([0, base_width, 0]) {
                rotate([0, 0, 270]) {
                makeCoverHolder(45);
            }
        }
    }
    
    translate([99, 0, 0]) {
        rotate([0, 0, 90]) {
            makeCoverHolder(6);
        }
    }
    
    translate([base_length, 0, 0]) {
        rotate([0, 0, 90]) {
            makeCoverHolder(6);
        }
    }
    
    translate([base_length - 105, base_width, 0]) {
        rotate([0, 0, 270]) {
            makeCoverHolder(105, false);
        }
    }
    
    // a "fence" at the top. This is not structural or to keep the
    // lexan in, it's just to keep crap out as much as we can
    translate([base_length - cover_support_thickness, 0, 0]) {
        cube([cover_support_thickness,
              100,
              base_height + base_thickness - lexan_thickness - cover_support_thickness]);
        translate([0, 100 - cover_support_thickness / 2, 0]) {
            rotate([0, 0, 180]) {
                // this wedge sticks out slightly less to leave room for the
                // buck converter, but it's not load bearing so this shouldn't be an issue.
                Wedge(a=base_height * .75,
                      b=10,
                      w1 = cover_support_thickness,
                      w2 = cover_support_thickness);
            }
        }
    }
}

module makeBaseMountingHoles() {
            translate([90, 90, -screw_hole_height/2]) {
            cylinder(d = anchor_screw_hole_diameter, h = screw_hole_height);
        }

        translate([170, 109, -screw_hole_height/2]) {
            cylinder(d = anchor_screw_hole_diameter, h = screw_hole_height);
        }

        translate([20, 165, -screw_hole_height/2]) {
            cylinder(d = anchor_screw_hole_diameter, h = screw_hole_height);
        }
}

// Lexan cover
module makeLexan() {
    color("azure",0.5) {
        translate([cover_support_thickness,
                   cover_support_thickness,
                   base_height + base_thickness - lexan_thickness - cover_support_thickness]) {
            // the box is open at the top (slides in) so the lexan is slightly rectangular
            cube([base_width - cover_support_thickness,
                  base_length - cover_support_thickness * 2, lexan_thickness]);
        }
    }
}


module makeEverything(showComponents = false) {
    union() {
        // base unit with screw holes through everything
        difference() {
            union() {
                makeBase();
                if (showComponents) {
                    color("blue") {
                        translate([100, 5, standoff_height + base_thickness]) {
                            makeSSRBoard();
                        }
                        
                        translate([18, 5, standoff_height + base_thickness]) {
                            makeSSRBoard();
                        }
                    
                        translate([65, 70, standoff_height + base_thickness]) {
                            rotate([0,0,90]) {
                                makeSSRBoard();
                            }
                        }
                    }
                    
                    translate([85, 115, standoff_height + base_thickness]) {
                        color("red") {
                            makeRPI();
                        }
                    }
                    
                    translate([120, 70, large_screw_standoff_height + 0.1]) {
                        color("purple") {
                            makeConverter();
                        }
                    }

                    translate([70, 150, standoff_height + base_thickness]) {
                        rotate([0,0,90]) {
                            color("green") {
                                make18B20Board();
                            }
                        }
                    }
                }
            }
            translate([100, 5, standoff_height + base_thickness]) {        
                makeSSRBoardHoles();
            }
            
            translate([18, 5, standoff_height + base_thickness]) {
                makeSSRBoardHoles();
            }

            translate([65, 70, standoff_height + base_thickness]) {
                rotate([0,0,90]) {
                    makeSSRBoardHoles();
                }
            }
            
            translate([85, 115, standoff_height + base_thickness]) {
                makeRPIBoardHoles();
            }
            
            translate([120, 70, standoff_height + base_thickness]) {
                makeConverterBoardHoles();
            }
            
            translate([70, 150, standoff_height + base_thickness]) {
                rotate([0,0,90]) {
                    make18B20BoardHoles();
                }
            }
            
            makeBaseMountingHoles();
        }
        if (showComponents) {
            // lexan sheet to simulate a pretty cover
            makeLexan();
        }
    }
}

makeEverything(false );

