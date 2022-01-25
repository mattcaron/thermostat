include <Chamfers-for-OpenSCAD/Chamfer.scad>;
include <triangles.scad>;

// Dimensions are in mm

// Fragment count. Use this many fragments to make a circle.
$fn = 360;
delta = 0.1;

// this is the gap between the "fingers" that hold the lid on,
// and the lid itself. We want this as tight as possible, so it
// doesn't fall off, but not so tight that you rip the sensor off
// the wall trying to pull it off.
wall_gap = 0.5;

enclosure_height = 70;
enclosure_length = 60;
enclosure_lid_depth = 30;
enclosure_base_depth = 4;
enclosure_wall_thickness = 2;
enclosure_divider_thickness = 1;

// there is this much subtracted from each edge of the divider
// so that it fits inside.
enclosure_divider_delta = enclosure_wall_thickness+wall_gap+3;

hole_spacing = 26.85;

// showAssembledUnit();

showItemsForPrinting();

// Render the unit, with internal components, assembled in situ.
// Comment out bits here to see how things are and do development.
module showAssembledUnit() {
    makeComponents();
    makeEnclosureLid();
    makeEnclosureBase();
    makeEnclosureDivider();
}

module showItemsForPrinting() {
/*
    translate([0,0,14]) {
        rotate([90,-90,0]) {
            makeEnclosureBase();
        }
    }
*/

    translate([0,0,20]) {
        rotate([90,90,0]) {
            makeEnclosureLid();
        }
    }

/*
    translate([0,0,-7]) {
        rotate([90,-90,0]) {
            makeEnclosureDivider();
        }
    }
*/
}

// Make the lid of the enclosure, which contains the
// components.
module makeEnclosureLid() {
    hole_diameter = 1.5;
    
    difference() {
        // basic hollow chamfered box
        translate([-10,
                   (-enclosure_length/2)+1,
                   (-enclosure_height/2)+1]) {
    
            difference() {
                chamferCube([enclosure_lid_depth,
                            enclosure_length-enclosure_wall_thickness,
                            enclosure_height-enclosure_wall_thickness],
                            ch=2,
                            chamfers=[[1, 1, 1, 1],
                                    [0, 0, 1, 1],
                                    [0, 1, 1, 0]]);
    
                translate([-enclosure_wall_thickness,
                            enclosure_wall_thickness/2,
                            enclosure_wall_thickness/2]) {
                    chamferCube([enclosure_lid_depth,
                                enclosure_length-
                                    enclosure_wall_thickness*2,
                                enclosure_height-
                                    enclosure_wall_thickness*2],
                                ch=2,
                                chamfers=[[1, 1, 1, 1],
                                        [0, 0, 1, 1],
                                        [0, 1, 1, 0]]);
                            }
                    }
            }
           
            // vents in box
            translate([enclosure_wall_thickness,
                       0,
                       -(enclosure_height/2-delta)]) {
                cube([enclosure_lid_depth/2,2,4], center=true);
            }
            translate([enclosure_wall_thickness,
                       4,
                       -(enclosure_height/2-delta)]) {
                cube([enclosure_lid_depth/2,2,4], center=true);
            }
            translate([enclosure_wall_thickness,
                       8,
                       -(enclosure_height/2-delta)]) {
                cube([enclosure_lid_depth/2,2,4], center=true);
            }
            translate([enclosure_wall_thickness,
                       -4,
                       -(enclosure_height/2-delta)]) {
                cube([enclosure_lid_depth/2,2,4], center=true);
            }
            translate([enclosure_wall_thickness,
                       -8,
                       -(enclosure_height/2-delta)]) {
                cube([enclosure_lid_depth/2,2,4], center=true);
            }
            translate([enclosure_wall_thickness,
                       0,
                       enclosure_height/2-delta]) {
                cube([enclosure_lid_depth/2,2,4], center=true);
            }
            translate([enclosure_wall_thickness,
                       4,
                       enclosure_height/2-delta]) {
                cube([enclosure_lid_depth/2,2,4], center=true);
            }
            translate([enclosure_wall_thickness,
                       8,
                       enclosure_height/2-delta]) {
                cube([enclosure_lid_depth/2,2,4], center=true);
            }
            translate([enclosure_wall_thickness,
                       -4,
                       enclosure_height/2-delta]) {
                cube([enclosure_lid_depth/2,2,4], center=true);
            }
            translate([enclosure_wall_thickness,
                       -8,
                       enclosure_height/2-delta]) {
                cube([enclosure_lid_depth/2,2,4], center=true);
            }            

    }
  
    
    // standoffs to which we screw the battery box
    difference() {
        translate([13, 0, 0]) {
            cube([enclosure_lid_depth - 20,
                  enclosure_length - enclosure_wall_thickness,
                  5],
                  center=true);
        }
        // holes
        translate([-10,
                   -(hole_spacing/2+hole_diameter),
                   0]) {
                 rotate([90,0,90]) {
                    cylinder(d=hole_diameter, h=20);
                 }
        }
        translate([-10,
                  hole_spacing/2+hole_diameter,
                  0]) {
                rotate([90,0,90]) {
                    cylinder(d=hole_diameter, h=20);
                 }
        }
        // wiring chase
        translate([10,0,0]) {
            cube([enclosure_lid_depth + delta - 15,10,12],center=true);
        }
    }
}

// Make the divider which separates the battery holder from
// the temperature sensor and PCB
module makeEnclosureDivider() {
    hole_diameter = 2;
    translate([7.8,0,0]) {
        difference() {
            cube([enclosure_divider_thickness,
                  enclosure_length - (enclosure_divider_delta * 2),
                  enclosure_height - (enclosure_divider_delta * 2)],
                  center=true);
            
            // holes
            translate([-10, -(hole_spacing/2+hole_diameter), 0]) {
                        rotate([90,0,90]) {
                        cylinder(d=hole_diameter, h=20);
                }
            }
            translate([-10, hole_spacing/2+hole_diameter, 0]) {
                rotate([90,0,90]) {
                    cylinder(d=hole_diameter, h=20);
                }
            }
        }
        
        // add in some clips
        
        // PCB clips, to prevent lateral movement.
        // Vertical movement is limited by the standoffs and lid
        
        // left side
        makeLeftSidePCBDividerClip();
        
        translate([0,0,30]) {
            rotate([180,0,0]) {
                makeLeftSidePCBDividerClip();
            }
        }
        
        // DS18B20 clips, to prevent lateral movement.
        // Vertical movement is limited by the lid and the force
        // of the wire pushing against the leads
        makeLeftSide18B20DividerClip();
        
        translate([0,0,-48]) {
            rotate([180,0,0]) {
                makeLeftSide18B20DividerClip();
            }
        } 
    }
}

// Make the left side PCB divider clip
module makeLeftSidePCBDividerClip() {
    translate([0, -19, 5]) {
        cube([7, 1, 20]);
    }
        
    translate([0, -20, 10]) {
        rotate([180,0,0]) {
            Right_Angled_Triangle(a=4,
                                  b=7,
                                  centerXYZ=[false,true,false]);
        }
    }
        
    translate([0, -20, 20]) {
        rotate([180,0,0]) {
            Right_Angled_Triangle(a=4,
                                  b=7,
                                  centerXYZ=[false,true,false]);
        }
    }
}

// Make the left side 18B20 divider clip
module makeLeftSide18B20DividerClip() {
    translate([0, -4, -29]) {
        cube([7, 1, 10]);
    }
        
    translate([0, -5, -24]) {
        rotate([180,0,0]) {
            Right_Angled_Triangle(a=4,
                                  b=7,
                                  centerXYZ=[false,true,false]);
        }
    }
}

// Make the base to which the lid connects
module makeEnclosureBase() {  
    rotate([0,0,180]) {
        translate([10,
                  (-enclosure_length/2)+1,
                  (-enclosure_height/2)+1]) {
            // basic box
            chamferCube([enclosure_base_depth,
                        enclosure_length-enclosure_wall_thickness,
                        enclosure_height-enclosure_wall_thickness],
                        ch=2,
                        chamfers=[[1, 1, 1, 1],
                                  [0, 0, 0, 0],
                                  [0, 0, 0, 0]]);
            // make corner pieces
            makeLowerCorners();
                      
            // upper pieces are the lower pieces, flipped and moved
            translate([0,
                       enclosure_length-enclosure_wall_thickness,
                       enclosure_height-enclosure_wall_thickness]) {
                rotate([180,0,0]) {
            
                    makeLowerCorners();
                }
            }
        }
    }
}

// Aggregate two calls to makeCorner with appropriate displacement
module makeLowerCorners() {
    makeCorner();
          
    translate([0,
               enclosure_length-(enclosure_wall_thickness-wall_gap),
               0]) {
        rotate([90,0,0]) {
            makeCorner();
        }
    } 
}


// Helper to make a base corner piece, placed at the lower right corner.
module makeCorner() {
    // the 1.5 here is a fudge. I am not proud.
    translate([0,
               enclosure_wall_thickness+wall_gap,
               enclosure_wall_thickness+wall_gap+1.5]) {
        rotate([270,180,0]) {
            rotate([135,0,0]) {
                cube([enclosure_lid_depth*0.75,2,1]);
            }
            translate([0,5.5,-0.5]) {
                Right_Angled_Triangle(a=enclosure_length/3,
                                      b=enclosure_lid_depth*0.75,
                                      centerXYZ=[false,true,false]);
            }
            translate([0,-1,7]) {
                rotate([90,0,0]){
                    Right_Angled_Triangle(a=enclosure_length/3,
                                          b=enclosure_lid_depth*0.75,
                                          centerXYZ=[false,true,false]);
                }
            }
        }
    }
}

// Show all our prefabricated components - that is,
// the ones which are to be fitted into the enclosure, in red.
module makeComponents() {
    color("red") {
        rotate([0,0,180]){
            translate([-10,0,-25]) {
                rotate([0,180,-90]) {
                    makeDS18B20();
                }
            }
            
           translate([-12,0,16]) {
                rotate([90,0,90]) {
                    makePCB();
                }
            }
            
            rotate([0,90,0]) {
                makeBatteryPack();
            }
        }
    }
}

// Describe a rough battery pack with no leads, but with screw
// holes at the correct place for lining up with the enclosure.
module makeBatteryPack() {
    l = 58.25;
    w = 48.15;
    h = 14.55;
    hole_diameter = 3.5;
    hole_down = 27.45;
    
    difference () {
        cube([l,w,h], center=true);
        translate([(l/2-hole_down-hole_diameter/2),
                    -(hole_spacing/2+hole_diameter/2),
                    -50]) {
            cylinder(d=hole_diameter, h=100);
        }
        translate([(l/2-hole_down-hole_diameter/2),
                    hole_spacing/2+hole_diameter/2,
                    -50]) {
            cylinder(d=hole_diameter, h=100);
        }
    }
}

// Describe a rough D1 PCB as a rectangle, no leads.
module makePCB() {
    l = 34.62;
    w = 25.60;
    h = 7.00;
    
    cube([l,w,h], center=true);
}

// Describe a rough DS18B20 with a short rectangle
// hanging off to block out the space for the leads.
module makeDS18B20(){
    w = 4.58;
    h = 4.58;
    leads_w = 5.5;
    leads_h = 15;
    leads_l = 1;
        union() {
        translate([-w/2,-0.5,0]){
            cube([w,2,h]);
        }
        difference() {
            cylinder(d=w, h=h);
            translate([-w/2,-0.5,-0.25]) {
                cube(5);
            }
        }
        // leads
        translate([-leads_w/2,0,-(leads_h-delta)]) {
            cube([leads_w, leads_l, leads_h]);
        }
    }
}