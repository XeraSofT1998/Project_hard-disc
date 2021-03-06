/**
 * @file    object.cpp
 * @author  James Sturgis
 * @date    April 6, 2018
 * @brief   Implementation of the object object.
 */

#include "object.h"
#include <math.h>
#include <float.h>
#include "common.h"
#include <boost/format.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

using boost::format;

/**
 * @brief Constructor of an empty object.
 */
object::object() {
    o_type       = -1;		/// Marks uninitialized invalid object
    pos_x        = 0.0;
    pos_y        = 0.0;
    orientation  = 0.0;
    recalculate  = true;
    saved_energy = 0.0;
    obj_n_good = 0;		/// accepted MC movement for the object
    obj_n_bad = 0;		/// refused MC movement for the object
    obj_n_rotation = 0;	/// number of rotation done by an object
    obj_n_translation = 0;	/// number of translation done by an object
    obj_dl_max = 0;		/// number of dl_max of an object
}

/**
 * @brief Constructor with known type, position and orientation.
 * @param type      Object type (should be known to topology).
 * @param x_pos     Object location (x direction).
 * @param y_pos     Object location (y direction).
 * @param angle     Object orientation angle.
 */
object::object(int  type, double x_pos, double y_pos, double angle ) {
    o_type       = type;
    pos_x        = x_pos;
    pos_y        = y_pos;
    orientation  = angle;
    recalculate  = true;
    saved_energy = 0.0;
    obj_n_good = 0;		/// accepted MC movement for the object
    obj_n_bad = 0;		/// refused MC movement for the object
    obj_n_rotation = 0;	/// number of rotation done by an object
    obj_n_translation = 0;	/// number of translation done by an object
    obj_dl_max = 0;		/// number of dl_max of an object
}

/**
 * @brief   Constructor for copying an object.
 * @param orig  The original object.
 */
object::object(const object& orig) {
    assign(orig);
}

/**
 * Destructor for objects
 */
object::~object() {
}

/**
 * @brief   Constructor for copying an object.
 * @param orig  The original object.
 */
void
object::assign(const object& orig) {
    o_type       = orig.o_type;
    pos_x        = orig.pos_x;
    pos_y        = orig.pos_y;
    orientation  = orig.orientation;
    recalculate  = true;            // Can't guarantee same context.
    saved_energy = 0.0;             // This does not mater.
    obj_n_good = orig.obj_n_good;
    obj_n_bad = orig.obj_n_bad;
    obj_n_rotation = orig.obj_n_rotation;
    obj_n_translation = orig.obj_n_translation;
    obj_dl_max = orig.obj_dl_max;
}

/**
 * @brief   Move an object in the distribution.
 *
 * @param max_dist  Maximum distance to move.
 * @param x_size    The boundary conditions for handling edges, width.
 * @param y_size    The boundary conditions for handling edges, height.
 * @param periodic  Use periodic conditions or a box?
 *
 * First calculate a distance to move from a levy distribution. Then chose an
 * angle so as to get the shifts dx and dy. Finally after moving the object
 * keep it in the boundary using either periodic conditions or reflective walls.
 */
void
object::move(double dx, double dy){
    /* Handle the edges to keep the object in the box */
    pos_x += dx;
    pos_y += dy;
    recalculate = true;
}

/**
 * Rotate a random object a random amount.
 * @param max_angle
 */
void    object::rotate(float angle){
    orientation += angle;
    recalculate = true;
    while( orientation < 0.0  ) orientation += M_2PI;
    while( orientation > M_2PI) orientation -= M_2PI;
}

/**
 * \brief Distance to a second object.
 *
 * Calculate the distance to obj2, using if there are periodic boundary conditions
 * the closest image of obj2. This is not really usefil for molecules but potentially
 * could help with exclude lists etc and optimization.
 *
 * @param obj2      The second object.
 * @param x_size    The width of the box
 * @param y_size    The height of the box
 * @param periodic  Flag for if there are periodic boundary conditions or not
 *
 * @return          The distance to object 2.
 *
 */
double  object::distance(object* obj2, double x_size, double y_size, bool periodic){
    double dx, dy, dx2, dy2;

    dx = pos_x - obj2->pos_x;
    dx *= dx;
    dy = pos_y - obj2->pos_y;
    dy *= dy;
    if(periodic){
        dx2 = pos_x + x_size;
        dx2 *= dx2;
        dx  = simple_min(dx, dx2);
        dx2 = pos_x - x_size;
        dx2 *= dx2;
        dx  = simple_min(dx, dx2);
        dy2 = pos_y + y_size;
        dy2 *= dy2;
        dy  = simple_min(dy, dy2 );
        dy2 = pos_y - y_size;
        dy2 *= dy2;
        dy  = simple_min(dy, dy2 );
    }
    return sqrt(dx+dy);
}

/**
 * @brief       Write an object to a file object.
 *
 * @param dest  The output stream to write to.
 * @return      Always returns true.
 */
int 
object::write(std::ostream& dest){
    dest << format("%5d %9f2 %9f2 %9f2\n") % o_type % pos_x % pos_y % orientation;
    return (int)true;
}

/**
 * @brief   Write an object to a file pointer.
 *
 * @param dest  The file to write to (opened for writing).
 * @return      The return value of the print statement.
 */
int object::write(FILE *dest){
    return( fprintf(dest,"%5d %9f2 %9f2 %9f2\n",
                 o_type, pos_x, pos_y, orientation ));
}

/**
 * @brief   Set the energy of an object to a new value.
 * @param new_energy    The new value for the energy.
 * @return              The value set (should be the same as that passed).
 */
double  object::set_energy(double new_energy){
    saved_energy = new_energy;
    recalculate  = false;
    return saved_energy;
}

double  object::get_energy(){
    assert(!recalculate);
    return saved_energy;
}

/**
 * @brief   Calculate the interaction energy with another object.
 * @param the_force       The force field to use for calculating the energy.
 * @param the_topologies  Topology information for the objects.
 * @param obj2            The second object with which this one is interacting.
 * @return                The calculated energy.
 *
 * For each atom in the first object calculate its position. Then for each atom
 * in the second object calculate its position. From the positions calculate
 * the interaction distance. Then use the force field to calculate the energy
 * given the distance.
 */
double  object::interaction(force_field* the_force,
                topology *the_topologies,
                object* obj2){
    int     i,j;
    int     n1;
    double  energy = 0.0;
    atom    *at1, *at2;
    double  x1, x2, dx, y1, y2, dy;
    double  distance;
    int     n2, t2;
    double  oo2, ox2, oy2;
    double  ax2, ay2;

    n1 = the_topologies->molecules(o_type).n_atoms;
    n2 = the_topologies->molecules(obj2->o_type).n_atoms;
    
    t2  = obj2->o_type;
    oo2 = obj2->orientation;
    ox2 = obj2->pos_x;
    oy2 = obj2->pos_y;

    for(i = 0; i < n1; i++){
        at1 = &(the_topologies->molecules(o_type).the_atoms(i));
        x1 = pos_x - sin(orientation)*at1->y_pos + cos(orientation)*at1->x_pos;
        y1 = pos_y + cos(orientation)*at1->y_pos + sin(orientation)*at1->x_pos;
        for(j = 0; j < n2; j++){ // This segment is the slowest...
            at2 = &(the_topologies->molecules(t2).the_atoms(j));
            ax2 = at2->x_pos;
            ay2 = at2->y_pos;
            x2 = ox2 - sin(oo2) * ay2 + cos(oo2)*ax2;
            y2 = oy2 + cos(oo2) * ay2 + sin(oo2)*ax2;
            dx = x2 - x1;
            dy = y2 - y1;
            distance = sqrt(dx*dx+dy*dy);
            energy += the_force->interaction(at1->type, i, at2->type, distance );
        }
    }
    return energy;
}

/**
 * For non periodic boundary conditions need to calculate energy of interaction
 * with the box. This could be extended to have a central attractor for example.
 * For that part of the energy should be moved to the force field interaction of
 * the atom with the box.
 *
 * @param the_force     The force field to use for calculating the energy.
 * @param the_topology  Topology information for the objects.
 * @param x_size        Width of the box
 * @param y_size        Height of the box.
 * @return              The interaction energy with the box.
 *
 * For each atom calculate its interaction with the box using the force_field.
 * Semmantically it should be the_force->interaction(atom, position, box);
 */
double  object::box_energy(
        force_field* the_force,
        topology* the_topology,
        double x_size, double y_size ){
    int     n1, i;
    double  x1, y1, r;
    atom    *at1;
    double  value = 0.0;

    n1 = the_topology->molecules(o_type).n_atoms;
    for(i=0;i<n1;i++){
        at1 = &(the_topology->molecules(o_type).the_atoms(i));
        x1 = pos_x - sin(orientation)*at1->y_pos + cos(orientation)*at1->x_pos;
        y1 = pos_y + cos(orientation)*at1->y_pos + sin(orientation)*at1->x_pos;
        r  = the_force->size(at1->type);
        if((x1 < r ) || (x1 > (x_size-r)) ||
                (y1 < r) || (x1 > (y_size-r))) value += the_force->big_energy;
    }
    return value;
}

double  object::box_energy(
	force_field *the_force,
        topology *the_topology,
        polygon *the_box ){
    int     n1, i;
    double  x1, y1, r;
    atom    *at1;
    double  value = 0.0;

    n1 = the_topology->molecules(o_type).n_atoms;
    for(i=0;i<n1;i++){
        at1 = &(the_topology->molecules(o_type).the_atoms(i));
        x1 = pos_x - sin(orientation)*at1->y_pos + cos(orientation)*at1->x_pos;
        y1 = pos_y + cos(orientation)*at1->y_pos + sin(orientation)*at1->x_pos;
        r  = the_force->size(at1->type);
        if(!the_box->is_inside(x1,y1,r)) value += the_force->big_energy;
    }
    return value;
}

/**
 * @brief   Increase all dimensions by multiplying by the factor dl.
 * @param   dl  Multiplication factor.
 */
void    object::expand(double dl){
    pos_x *= dl;
    pos_y *= dl;
}
