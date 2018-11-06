/* 
RMCIOS - Reactive Multipurpose Control Input Output System
Copyright (c) 2018 Frans Korhonen

RMCIOS was originally developed at Institute for Atmospheric 
and Earth System Research / Physics, Faculty of Science, 
University of Helsinki, Finland

Assistance, experience and feedback from following persons have been 
critical for development of RMCIOS: Erkki Siivola, Juha Kangasluoma, 
Lauri Ahonen, Ella Häkkinen, Pasi Aalto, Joonas Enroth, Runlong Cai, 
Markku Kulmala and Tuukka Petäjä.

This file is part of RMCIOS. This notice was encoded using utf-8.

RMCIOS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RMCIOS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public Licenses
along with RMCIOS.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "RMCIOS-functions.h"

float plus_func (float a, float b)
{
   return a + b;
}
float minus_func (float a, float b)
{
   return a - b;
}
float multiply_func (float a, float b)
{
   return a * b;
}
float divide_func (float a, float b)
{
   return a / b;
}
float pow2_func (float a, float b)
{
   return a * a;
}

struct oper
{
   float valueA;
   float valueB;
   int valueA_channel;
   int valueB_channel;
};

void generic_operator_class_func (struct oper *this,
                                  const struct context_rmcios *context,
                                  int id, enum function_rmcios function,
                                  enum type_rmcios paramtype,
                                  union param_rmcios returnv, int num_params,
                                  const union param_rmcios param,
                                  float (*opfunc) (float, float))
{
   switch (function)
   {
   case setup_rmcios:
      if (this == 0)
         break;
      if (num_params < 1)
         break;
      this->valueB = param_to_float (context, paramtype, param, 0);
      if (num_params < 2)
         break;
      this->valueA_channel = param_to_int (context, paramtype, param, 1);
      if (num_params < 3)
         break;
      this->valueB_channel = param_to_int (context, paramtype, param, 2);
      break;

   case write_rmcios:
      if (this == 0)
         break;
      if (this->valueA_channel != 0)
         // update A from channel
         this->valueA = read_f (context, this->valueA_channel); 
      if (this->valueB_channel != 0)
         // update B from channel
         this->valueB = read_f (context, this->valueB_channel); 
      if (num_params >= 1)
         this->valueA = param_to_float (context, paramtype, param, 0);
      write_f (context, linked_channels (context, id),
               opfunc (this->valueA, this->valueB));
      break;
   case read_rmcios:
      if (this == 0)
         break;
      if (this->valueA_channel != 0)
         // update A from channel
         this->valueA = read_f (context, this->valueA_channel); 
      if (this->valueB_channel != 0)
         // update B from channel
         this->valueB = read_f (context, this->valueB_channel); 
      return_float (context, paramtype, returnv,
                    opfunc (this->valueA, this->valueB));
      break;
   default:
      break;
   }
}

///////////////////////////////////////////////
//! Channel for plus operation
///////////////////////////////////////////////
void plus_class_func (struct oper *this,
                      const struct context_rmcios *context, int id,
                      enum function_rmcios function,
                      enum type_rmcios paramtype,
                      union param_rmcios returnv,
                      int num_params, const union param_rmcios param)
{
   switch (function)
   {
   case help_rmcios:
      return_string (context, paramtype, returnv,
                     "help for plus channel\r\n"
                     "calculates A+B\r\n"
                     " create plus newname\r\n"
                     " setup newname B | valueA_channel | valueB_channel "
                     "#Set constant value B, or overriding value channels.\r\n"
                     " link newname channel #link result to channel\r\n"
                     " write newname A # Calculate:A+B=result \r\n"
                     " read newname #read result \r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      // allocate new data
      this = (struct oper *) 
              allocate_storage (context, sizeof (struct oper), 0);       

      //default values :
      this->valueA = 0;
      this->valueB = 0;
      this->valueA_channel = 0;
      this->valueB_channel = 0;

      // create channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) plus_class_func, this); 
      break;
   }

   // use generic implementation
   generic_operator_class_func (this, context, id, function, paramtype, returnv, num_params, param, plus_func); 
}


///////////////////////////////////////////////
//! Channel for minus operation
///////////////////////////////////////////////
void minus_class_func (struct oper *this,
                       const struct context_rmcios *context, int id,
                       enum function_rmcios function,
                       enum type_rmcios paramtype,
                       union param_rmcios returnv,
                       int num_params, const union param_rmcios param)
{
   switch (function)
   {
   case help_rmcios:
      return_string (context, paramtype, returnv,
                     "help for minus channel\r\n"
                     "A-B\r\n"
                     " create minus newname\r\n"
                     " setup newname B #Set value B\r\n"
                     " link newname channel #link result to channel\r\n"
                     " write newname A # Calculate:A-B=result \r\n"
                     " read newname #read result \r\n");
      break;
   case create_rmcios:
      if (num_params < 1)
         break;

      this = (struct oper *) allocate_storage (context, sizeof (struct oper), 0);       // allocate new data
      //default values :
      this->valueA = 0;
      this->valueB = 0;
      this->valueA_channel = 0;
      this->valueB_channel = 0;

      // create channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) minus_class_func, this);       
      break;
   }
   // use generic implementation      
   generic_operator_class_func (this, context, id, function, paramtype, 
                                returnv, num_params, param, minus_func);        
}


///////////////////////////////////////////////
//! Channel for multiply operation
///////////////////////////////////////////////
void multiply_class_func (struct oper *this,
                          const struct context_rmcios *context, int id,
                          enum function_rmcios function,
                          enum type_rmcios paramtype,
                          union param_rmcios returnv,
                          int num_params, const union param_rmcios param)
{
   switch (function)
   {
   case help_rmcios:
      return_string (context, paramtype, returnv,
                     "help for multiply channel\r\n"
                     "A*B\r\n"
                     " create multiply newname\r\n"
                     " setup newname B #Set value B\r\n"
                     " write newname A # Calculate:A*B=result \r\n"
                     " read newname #read result \r\n"
                     " link newname channel #link result to channel\r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      // allocate new data
      this = (struct oper *) allocate_storage (context, sizeof (struct oper), 0);

      //default values :
      this->valueA = 0;
      this->valueB = 1;
      this->valueA_channel = 0;
      this->valueB_channel = 0;
      // create channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) multiply_class_func, this);    
      break;

   }
   // use generic implementation
   generic_operator_class_func (this, context, id, function, paramtype, 
                                returnv, num_params, param, multiply_func);     
}


///////////////////////////////////////////////
//! Channel for divide operation
///////////////////////////////////////////////
void divide_class_func (struct oper *this,
                        const struct context_rmcios *context, int id,
                        enum function_rmcios function,
                        enum type_rmcios paramtype,
                        union param_rmcios returnv,
                        int num_params, const union param_rmcios param)
{
   switch (function)
   {
   case help_rmcios:
      return_string (context, paramtype, returnv,
                     "help for divide channel:\r\n"
                     "A/B\r\n"
                     " create divide newname\r\n"
                     " setup newname B | valueA_channel valueB_channel "
                     "#Set constant value B, or overriding value channels.\r\n"
                     " write newname A # Calculate:A/B=result \r\n"
                     " read newname #read result \r\n"
                     " link newname channel #link result to channel\r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      // allocate new data
      this = (struct oper *) allocate_storage (context, sizeof (struct oper), 0);

      //default values :
      this->valueA = 0;
      this->valueB = 1;
      this->valueA_channel = 0;
      this->valueB_channel = 0;

      // create channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) divide_class_func, this);      
      break;
   }
   // use generic implementation
   generic_operator_class_func (this, context, id, function, paramtype, 
                                returnv, num_params, param, divide_func);
}


///////////////////////////////////////////////
//! Channel for power of 2 operation
///////////////////////////////////////////////
void pow2_class_func (struct oper *this,
                      const struct context_rmcios *context, int id,
                      enum function_rmcios function,
                      enum type_rmcios paramtype,
                      union param_rmcios returnv,
                      int num_params, const union param_rmcios param)
{
   //int create =1;
   switch (function)
   {
   case help_rmcios:
      return_string (context, paramtype, returnv,
                     "help for power of 2:\r\n"
                     "create pow2 newname\r\n"
                     "write newname value # calulate result=value^2 \r\n"
                     "read newname # read the result\r\n"
                     "link channel # link result to channel\r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      // allocate new data
      this = (struct oper *) allocate_storage (context, sizeof (struct oper), 0);       
      //default values :
      this->valueA = 1;
      this->valueB = 1;
      this->valueA_channel = 0;
      this->valueB_channel = 0;
      // create channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) pow2_class_func, this); 
      break;
   }

   // use generic implementation
   generic_operator_class_func (this, context, id, function, paramtype, 
                                returnv, num_params, param, pow2_func); 
}

////////////////////////////////////////////////////////
// Channel for linear fit (linear fit with 2 points)
////////////////////////////////////////////////////////
// y=y1+(y2-y1)*(x-x1)/(x2-x1)
struct linear_interpolation_data
{
   float x1;
   float y1;
   float x2;
   float y2;
   float y;
};

void linear_interpolation_class_func (struct linear_interpolation_data *this,
                                      const struct context_rmcios *context,
                                      int id, enum function_rmcios function,
                                      enum type_rmcios paramtype,
                                      union param_rmcios returnv,
                                      int num_params,
                                      const union param_rmcios param)
{
   switch (function)
   {
   case help_rmcios:
      return_string (context, paramtype, returnv,
                     "help for linear interpolation channel:\r\n"
                     "create linear_interpolation newname\r\n"
                     "setup newname x1 y1 x2 y2 "
                     "# define 2 point from straight line. x=input y=output \r\n"
                     "write newname x # calculate y=f(x) \r\n"
                     "read newname # read the result \r\n"
                     "link channel # link result to channel\r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      // allocate new data
      this = (struct linear_interpolation_data *) 
             allocate_storage (context, 
                               sizeof(struct linear_interpolation_data), 0);     

      //default values :
      this->x1 = 0;
      this->y1 = 0;
      this->x2 = 1;
      this->y2 = 1;
      this->y = 0;
      
      // create channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) linear_interpolation_class_func, 
                            this);        
   case setup_rmcios:
      if (this == 0)
         break;
      if (num_params < 4)
         break;
      this->x1 = param_to_float (context, paramtype, param, 0);
      this->y1 = param_to_float (context, paramtype, param, 1);
      this->x2 = param_to_float (context, paramtype, param, 2);
      this->y2 = param_to_float (context, paramtype, param, 3);
      break;
   case write_rmcios:
      if (this == 0)
         break;
      if (num_params < 1)
         break;
      {
         float x = param_to_float (context, paramtype, param, 0);
         this->y = this->y1 + (this->y2 - this->y1) *
            (x - this->x1) / (this->x2 - this->x1);
         write_f (context, linked_channels (context, id), this->y);
      }
      break;
   case read_rmcios:
      if (this == 0)
         break;
      return_float (context, paramtype, returnv, this->y);
      break;
   }
}

/////////////////////////////////
//! Channel for averaging data //
/////////////////////////////////
struct average_data
{
   float sum;                   // average sum
   float trigger_start;         // start of user triggered average
   int sum_items;               // user triggered sum items
   int cyclic_index;            // items added to cyclic sum
   unsigned int cyclic_count;   // number of values to sum 
   float average;
};

void average_class_func (struct average_data *this,
                         const struct context_rmcios *context, int id,
                         enum function_rmcios function,
                         enum type_rmcios paramtype,
                         union param_rmcios returnv,
                         int num_params, const union param_rmcios param)
{
   switch (function)
   {
   case help_rmcios:
      return_string (context, paramtype, returnv,
                     "average - Channel for calculating average."
                     " (cyclic and user triggered)\r\n"
                     " create average newname\r\n"
                     " setup newname n"
                     " #sets n number of values for cyclic average.\r\n"
                     "    #if n=0 sends average to linked on empty write.\r\n"
                     " write newname value #adds value to average sum."
                     " After n values sends average to linked channel.\r\n"
                     " read newname #returns average since last empty write\r\n"
                     " write newname #returns average since last empty write\r\n"
                     "       #Resets the user triggered average. "
                     "Restarts cyclic average discarding latest sum (if n>0)\r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      // allocate new data
      this = (struct average_data *) 
             allocate_storage (context, sizeof (struct average_data), 0);       

      //default values :
      this->sum = 0.0;
      this->sum_items = 0;
      this->trigger_start = 0.0;
      this->cyclic_index = 0;
      this->cyclic_count = 0;
      // create channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) average_class_func, this);     
      break;

   case setup_rmcios:
      if (this == 0)
         break;
      if (num_params < 1)
         break;
      else
      {
         this->cyclic_count = param_to_int (context, paramtype, param, 0);
         this->cyclic_index = 0;
      }
      break;

   case write_rmcios:
      if (this == 0)
         break;
      if (num_params < 1)      
      // Empty write (reset average)
      {
         this->average = (this->sum - this->trigger_start) / this->sum_items;

         if (this->cyclic_count == 0)
            write_f (context, linked_channels (context, id), this->average);

         this->cyclic_index = 0;
         this->trigger_start = 0;
         this->sum = 0;
         this->sum_items = 0;

         return_float (context, paramtype, returnv, this->average);
         break;
      }

      this->sum += param_to_float (context, paramtype, param, 0);
      this->sum_items++;
      this->average = (this->sum - this->trigger_start) / this->sum_items;

      if (this->cyclic_count > 0)
      {
         this->cyclic_index++;
         if (this->cyclic_index >= this->cyclic_count)
         {
            write_f (context, linked_channels (context, id),
                     this->sum / this->cyclic_count);
            this->cyclic_index = 0;
            this->trigger_start -= this->sum;
            this->sum = 0;
         }
      }
      break;

   case read_rmcios:
      if (this == 0)
         break;
      else
      {
         return_float (context, paramtype, returnv, this->average);
      }
      break;
   }
}

//////////////////////////////////
//! Channel for summing numbers //
//////////////////////////////////
struct sum_data
{
   // average sum
   float sum;                   
};

void sum_class_func (struct sum_data *this,
                     const struct context_rmcios *context, int id,
                     enum function_rmcios function,
                     enum type_rmcios paramtype,
                     union param_rmcios returnv,
                     int num_params, const union param_rmcios param)
{
   switch (function)
   {
   case help_rmcios:
      return_string (context, paramtype, returnv,
                     "average - Channel for calculating sum."
                     " create sum newname\r\n"
                     " write newname value #adds value to sum."
                     " writes sum to linked channels.\r\n"
                     " read newname #returns the sum\r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      
      // allocate new data
      this = (struct sum_data *) 
             allocate_storage (context, sizeof (struct sum_data), 0);       

      //default values :
      this->sum = 0.0;
      // create channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) sum_class_func, this); 
      break;
   case write_rmcios:
      if (this == 0)
         break;
      if (num_params < 1)
         break;

      this->sum += param_to_float (context, paramtype, param, 0);
      write_f (context, linked_channels (context, id), this->sum);

      break;

   case read_rmcios:
      if (this == 0)
         break;
      else
      {
         return_float (context, paramtype, returnv, this->sum);
      }
      break;
   }
}

void init_math_channels (const struct context_rmcios *context)
{
   // Math_channels
   create_channel_str (context, "plus", (class_rmcios) plus_class_func, 0);
   create_channel_str (context, "minus", (class_rmcios) minus_class_func, 0);
   create_channel_str (context, "multiply",
                       (class_rmcios) multiply_class_func, 0);
   create_channel_str (context, "divide", (class_rmcios) divide_class_func, 0);
   create_channel_str (context, "pow2", (class_rmcios) pow2_class_func, 0);
   create_channel_str (context, "interpolation",
                       (class_rmcios) linear_interpolation_class_func, 0);
   create_channel_str (context, "average", (class_rmcios) average_class_func,
                       0);
   create_channel_str (context, "sum", (class_rmcios) sum_class_func, 0);
}
