
// Dimensions
box_thick = 2;
box_width = 70 + 2 *  box_thick;
box_height = 60 + 2 *  box_thick;
box_depth = 30 + 2 *  box_thick;
connector_height = 10;
connector_width = 10;

// Bottom
difference() {
    // Parte exterior - interior (caja hueca)
    cube([box_width,  box_height,  box_depth]);
    translate([box_thick,  box_thick, 2 *  box_thick])
      cube([box_width - 2 * box_thick, box_height - 2 * box_thick, box_depth ]);
}

// Top
translate([100,0,0]) {
    difference() {
        // Tapa + reborde de tapa
        union() {
            cube([box_width, box_height, box_thick]);
            translate([box_thick, box_thick, 0])
              cube([box_width - 2 * box_thick, box_height- 2 * box_thick, 2 * box_thick]);
        }

        // Grid hexagonal tapa
        translate([box_thick + 1, box_thick + 1, -1])
          grid_hexagonal(box_width - (2 * box_thick), box_height - (2 * box_thick ), box_thick + 2, 16);
    
        // Connector hole - top right corner (with margin from edge)
translate([box_width - connector_width - box_thick - 3, box_height - connector_height - box_thick - 3, -1])
  cube([connector_width, connector_height, 2 * box_thick + 2]);
    }  
}
       

module grid_hexagonal(width, height, depth , hexagons_width) {    
    separation = (width / hexagons_width ) / 2.25;
    intersection() {
       // cube([width,height,depth], center = false);
        translate( [ separation / 2, separation / 2, 0] ) intersection() {
            array_hexagons(width - 4, height - 4, depth + 2, hexagons_width );
            cube([width - 4,height - 4,depth + 2], center = false);
        }
    }
}

/**
 * Create an array of hexagons
 */
module array_hexagons(width, height, depth, hexagons_width) {    
    separation = (width / hexagons_width);
    hexagons_height = (hexagons_width * height / width);
    radio = separation / 2.25;
    
    for( x = [ 0 : 1 : hexagons_width])
        for( y = [ 0 : 1 : hexagons_height ])
            translate([(x * separation), ( y + ( x % 2 )/2 ) * separation ,0])
                cylinder(h = depth, r1 = radio, r2 = radio, center = false , $fn = 6);

}
