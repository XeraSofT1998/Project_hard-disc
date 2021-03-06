/**
 * @file    object.h
 * @author  James Sturgis
 * @date    April 6, 2018
 * \brief   Header file for the object class.
 *
 * @class   object object.h
 * \brief   An object in a configuration.
 *
 */

#ifndef OBJECT_H
#define OBJECT_H

#include "force_field.h"
#include "topology.h"
#include "polygon.h"
#include <fstream>
#include <sstream>

class object {
public:
    object();				    ///< Constructor empty (invalid) object.
    object(int type, double x_pos,
           double y_pos, double angle );    ///< Constructor with explicit properties.
    object(const object& orig);             ///< Copy constructor

    virtual ~object();                      ///< Destructor

    void    assign(const object &orig);	    ///< Copy assignment

    void    move(double dx, double dy);     ///< Move the object by dx, dy
    void    rotate(float angle );          ///< Rotate the object angle.
    int     write(std::ostream& _out);      ///< Write the object to a file
    int     write(FILE *dest);              ///< Write the object to a file
    double  distance(object *obj2, double x_size,
                     double y_size,
                     bool periodic);        ///< Return shortest distance to second object (using periodic conditions if necessary)
    double  set_energy(double new_energy);  ///< Set the energy of the object.
    double  get_energy();                   ///< Get the energy of the object.
    void    expand(double dl);              ///< Move coordinates by multiplication with dl.
    double  interaction(force_field *the_force,
                topology *the_topology,
                object *obj2);              ///< The energy of interaction with obj2
    double  box_energy(force_field *the_force,
                topology *the_topology,
                double x_size,
                double y_size );            ///< The energy due to the position in the box.
    double  box_energy(force_field *the_force,
                topology *the_topology,
                polygon *the_box );         ///< The energy due to the position in the box.
    bool    recalculate;                    ///< Flag set if energy needs recalculation
    double  pos_x, pos_y;                   ///< Position of the object
    double  orientation;                    ///< Rotational orientation
    int     o_type;                         ///< Type of object determines atoms.
    int  obj_n_good;	      	     	     ///<  MC ratio of good move of each object 
    int  obj_n_bad;		    	     ///<  MC ratio of bad move of each object
    int  obj_n_rotation;		     ///<  number of rotation for this object.
    int  obj_n_translation;		     ///<  number of translation of an object.
    float  obj_dl_max;  		     ///<  individual dl_max of the object. 
    
private:
    double  saved_energy;                   ///< Short cut if no need to recalculate


};

#endif /* OBJECT_H */

