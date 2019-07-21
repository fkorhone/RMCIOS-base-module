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


/* 2017 Frans Korhonen. University of Helsinki. 
 * Standard channels module implementation. 
 *
 * Changelog: (date,who,description)
 * */

#include "RMCIOS-functions.h"

/* Compare strings (glibc)*/
static int strcmp (const char *p1, const char *p2)
{
   const unsigned char *s1 = (const unsigned char *) p1;
   const unsigned char *s2 = (const unsigned char *) p2;
   unsigned char c1, c2;

   do
   {
      c1 = (unsigned char) *s1++;
      c2 = (unsigned char) *s2++;
      if (c1 == '\0')
         return c1 - c2;
   }
   while (c1 == c2);

   return c1 - c2;
}

/////////////////////////////////////////////////////////////////
/// @brief Channel for grouping links
////////////////////////////////////////////////////////////////
void joint_class_func (void *data,
                       const struct context_rmcios *context, int id,
                       enum function_rmcios function,
                       enum type_rmcios paramtype,
                       struct combo_rmcios *returnv,
                       int num_params, const union param_rmcios param)
{
   switch (function)
   {
   case help_rmcios:
      return_string (context, returnv,
                     "joint channel for grouping links.\r\n"
                     " create joint newname\r\n"
                     " setup newname params #call setup on linked channels\r\n"
                     " write newname data #write to linked channels\r\n"
                     " read newname #read from linked channels\r\n"
                     " link newname channel #link a channel \r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      // create channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) joint_class_func, 0);  
      break;

   case setup_rmcios:
   case read_rmcios:
   case write_rmcios:
      context->run_channel (context, linked_channels (context, id),
                            function, paramtype, returnv, num_params, param);
      break;
   }
}

///////////////////////////////////////////////////////////////////////////////
// Channel for converting parametrised call into triggering call (no parameters)
///////////////////////////////////////////////////////////////////////////////

struct trigger_data
{
   enum
   { TRIGGER_ALL = 0, TRIGGER_EMPTY = 1 } mode;
};


void trigger_class_func (struct trigger_data *this,
                         const struct context_rmcios *context, int id,
                         enum function_rmcios function,
                         enum type_rmcios paramtype,
                         struct combo_rmcios *returnv,
                         int num_params, const union param_rmcios param)
{
   switch (function)
   {
   case help_rmcios:
      return_string (context, returnv,
                     "trigger channel for convering data into triggers. "
                     "(calls without parameters) \r\n"
                     " create trigger newname #create new trigger\r\n"
                     " setup newname trigger_type(all) #setup trigger type:\r\n"
                     "  all # Triggers on everything\r\n"
                     "  empty # Triggers on empty write\r\n"
                     " read newname data"
                     " #make empty read to linked channels\r\n"
                     " write newname data "
                     "#make empty write to linked channels\r\n"
                     " link newname channel #link a channel \r\n");
      break;
   case create_rmcios:
      if (num_params < 1)
         break;
      // allocate new data
      this = (struct trigger_data *) 
             allocate_storage (context, sizeof (struct trigger_data), 0); 
      if (this == 0)
         break;

      this->mode = TRIGGER_ALL;

      // create channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) trigger_class_func, this); 
      break;

   case setup_rmcios:
      if (this == 0)
         break;
      else if (num_params >= 1)
      {
         char buffer[10];
         const char *s;
         s = param_to_string (context, paramtype, param, 0,
                              sizeof (buffer), buffer);
         if (strcmp (s, "all") == 0)
            this->mode = TRIGGER_ALL;
         if (strcmp (s, "empty") == 0)
            this->mode = TRIGGER_EMPTY;
      }

      break;

   case write_rmcios:
   case read_rmcios:
      if (this == 0)
         break;
      switch (this->mode)
      {
      case TRIGGER_EMPTY:
         if (num_params > 0)
            break;
      case TRIGGER_ALL:
         context->run_channel (context, linked_channels (context, id),
                               function,
                               paramtype,
                               0,
                               0, (const union param_rmcios) 0);
         break;
      }
      break;
   }
}

/////////////////////////////////////////////////////////////
//! Channel for printing system compile time date and time //
/////////////////////////////////////////////////////////////
void version_class_func (void *data,
                         const struct context_rmcios *context, int id,
                         enum function_rmcios function,
                         enum type_rmcios paramtype,
                         struct combo_rmcios *returnv,
                         int num_params, const union param_rmcios param)
{
   switch (function)
   {
   case read_rmcios:
      {
         return_string (context, returnv,
                        "compile time:" __DATE__ " " __TIME__);
      }
   }
}

////////////////////////////////////////////////////////
// Float channel
///////////////////////////////////////////////////////
struct float_data
{
   float value;
};

void float_class_func (struct float_data *this,
                       const struct context_rmcios *context, int id,
                       enum function_rmcios function,
                       enum type_rmcios paramtype,
                       struct combo_rmcios *returnv,
                       int num_params, const union param_rmcios param)
{
   switch (function)
   {
   case help_rmcios:
      return_string (context, returnv,
                     "Float channel -" 
                     " Channel for storing and sending float value. \r\n"
                     " create float newname\r\n"
                     " write float value\r\n"
                     "  -returns float representation of value\r\n"
                     " setup newname float_value \r\n"
                     " read newname\r\n"
                     " write newname\r\n"
                     "  -Send float to linked channels\r\n"
                     " write newname float\r\n"
                     "  -Set the float and send to linked channels. \r\n"
                     " link newname channel \r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      this = (struct float_data *) 
             allocate_storage (context, sizeof (struct float_data), 0); 
      
      if (this == 0)
         break;
      this->value = 0;
      
      // create channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) float_class_func, this); 
      break;

   case setup_rmcios:
      if (this == 0)
         break;
      if (num_params < 1)
         break;
      this->value = param_to_float (context, paramtype, param, 0);
      break;

   case read_rmcios:
      if (this == 0)
         break;
      return_float (context, returnv, this->value);
      break;
   case write_rmcios:
      if (num_params > 0)
      {
         if (this == 0)
         {
            return_float (context, returnv, 
                          param_to_float (context, paramtype, param, 0) );
            break;
         }
         this->value = param_to_float (context, paramtype, param, 0);
      }
      write_f (context, linked_channels (context, id), this->value);
      break;
   }
}

//////////////////////////////////
// Channel for storing integers //
//////////////////////////////////
struct int_data
{
   int value;
};

void int_class_func (struct int_data *this,
                     const struct context_rmcios *context, int id,
                     enum function_rmcios function,
                     enum type_rmcios paramtype,
                     struct combo_rmcios *returnv,
                     int num_params, const union param_rmcios param)
{
   switch (function)
   {
   case help_rmcios:
      return_string (context, returnv,
                     "Int channel - "
                     "Channel for storing and sending integer value. \r\n"
                     " create int newname\r\n"
                     " setup newname int_value \r\n"
                     " read newname\r\n"
                     " write newname\r\n"
                     "  -Send integer to linked channels\r\n"
                     " write newname integer\r\n"
                     "  -Set the integer and send to linked channels. \r\n"
                     " link newname channel \r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      this = (struct int_data *) 
             allocate_storage (context, sizeof (struct int_data), 0); 

      if (this == 0)
         break;
      this->value = 0;
      // create channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) int_class_func, this); 
      break;

   case setup_rmcios:
      if (this == 0)
         break;
      if (num_params < 1)
         break;
      this->value = param_to_int (context, paramtype, param, 0);
      break;

   case read_rmcios:
      if (this == 0)
         break;
      return_int (context, returnv, this->value);
      break;
   case write_rmcios:
  
      if (num_params > 0)
      {
         if (this == 0)
         {  
               return_int (context, returnv, 
                     param_to_int (context, paramtype, param, 0));
            break;
         }

         this->value = param_to_int (context, paramtype, param, 0);
      }
      write_i (context, linked_channels (context, id), this->value);
      break;
   }
}

/////////////////////////////////
// Channel for storing strings //
/////////////////////////////////
struct string_data
{
   char *buffer;
   int buffer_size;
};

void string_class_func (struct string_data *this,
                        const struct context_rmcios *context, int id,
                        enum function_rmcios function,
                        enum type_rmcios paramtype,
                        struct combo_rmcios *returnv,
                        int num_params, const union param_rmcios param)
{
   switch (function)
   {
   case help_rmcios:
      return_string (context, returnv,
                     "String channel -"
                     " Channel for storing and sending strings. \r\n"
                     " create string newname\r\n"
                     " setup newname string_value | max_chars\r\n"
                     " -Set string and optionally the buffer size. \r\n"
                     " -Allocates more when string_value is bigger max_chars\r\n"
                     " read newname\r\n"
                     "  -Read the stored string\r\n"
                     " write newname"
                     "  -Send string to linked channels\r\n"
                     " write newname string\r\n"
                     "  -Set the string and send to linked channels. \r\n"
                     "  -Strings bigger than max_chars will be truncated.\r\n"
                     " link newname channel \r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      // allocate new data
      this = (struct string_data *) 
             allocate_storage (context, sizeof (struct string_data), 0); 
      
      if (this == 0)
         break;
      this->buffer_size = 32;
      this->buffer = (char *) allocate_storage (context, this->buffer_size, 0);
      if (this->buffer == 0)
         this->buffer_size = 0;
      else
         this->buffer[0] = 0;

      // create channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) string_class_func, this); 
      break;

   case setup_rmcios:
      if (this == 0)
         break;

      if (num_params < 1)
         break;
      {
         int pslen = param_string_length (context, paramtype, param, 0) + 1;
         int max_size = 0;
         if (num_params >= 2)
            max_size = param_to_int (context, paramtype, param, 1) + 1;
         if (pslen > max_size)
            // String bigger than requested buffer
            max_size = pslen;   

         if (max_size > this->buffer_size)      
         // Needed buffer is bigger than old buffer.
         {
            this->buffer_size = max_size;
            free_storage (context, this->buffer, 0);
            this->buffer =
               (char *) allocate_storage (context, this->buffer_size, 0);
         }
         param_to_string (context, paramtype, param, 0,
                          this->buffer_size, this->buffer);
      }
      break;

   case read_rmcios:
      if (this == 0)
         break;
      return_string (context, returnv, this->buffer);
      break;
   case write_rmcios:
      if (num_params > 0)
      {
         if (this == 0)
         {
            int blen=param_string_alloc_size(context, paramtype, param,0) ;
            {
               char buffer[blen] ;
               const char *s= param_to_string (context, paramtype, param, 0,
                                               blen, buffer) ;
               return_string(context, returnv, s);
            }
            break ;
         }
         param_to_string (context, paramtype, param, 0,
                          this->buffer_size, this->buffer);
      }
      write_str (context, linked_channels (context, id), this->buffer, 0);
      break;
   }
}

//////////////////////////////////////////////////////////
// Channel for making call chains between channels.     //
//////////////////////////////////////////////////////////
void chain_class_func (void *this,
                       const struct context_rmcios *context, int id,
                       enum function_rmcios function,
                       enum type_rmcios paramtype,
                       struct combo_rmcios *returnv,
                       int num_params, const union param_rmcios param)
{
   int i, ch;
   switch (function)
   {
   case help_rmcios:
      return_string (context, returnv,
                     "chain channel - form chains of channel calls \r\n"
                     "create chain ch1 ch2 ch3 ch4...\r\n"
                     "means:\r\n"
                     "link ch1 ch2\r\n"
                     "link ch2 ch3\r\n" "link ch3 ch4\r\n" "...\r\n");
      break;

   case create_rmcios:
      ch = 0;
      if (num_params < 2)
         break;
      for (i = 1; i < num_params; i++)
      {
         link_channel_function (context,
                                param_to_int (context, paramtype,
                                              param, ch),
                                param_to_int (context, paramtype,
                                              param, i), 0, 0);
         ch = i;
      }
      break;
   }
}

void binary_class_func (void *this,
                        const struct context_rmcios *context, int id,
                        enum function_rmcios function,
                        enum type_rmcios paramtype,
                        struct combo_rmcios *returnv,
                        int num_params, const union param_rmcios param)
{
   switch (function)
   {
   case help_rmcios:
      return_string (context, returnv,
                     "binary channel -"
                     " read and convert binary data byte by byte."
                     "Multiple byte values are processed in Big-endian form.\r\n"
                     "create binary newname\r\n"
                     "setup newname bit_offset bit_count | bits1 channel1 |"
                     " bits2 channel2 | bits3 channel3 ... \r\n"
                     "   -Reception starts at bit_offset "
                     "and stops after bit count. \r\n"
                     "   -After last channel the reception continues"
                     " with first channel field. \r\n"
                     "   -Setting bit_count to 0 means continuous"
                     " readout (loops forever)"
                     "   -field bit witdths and channels can be used for "
                     " both direction conversions"
                     "write newname\r\n"
                     " -reset reception counters to 0\r\n"
                     "write newname data\r\n"
                     " -write binary data to convert from\r\n"
                     "read newname"
                     "  -read binary data formed from configured channels data.\r\n");
      break;

   }
}

void init_system_channels (const struct context_rmcios *context)
{
   // System channels
   create_channel_str (context, "joint", (class_rmcios) joint_class_func, 0);
   create_channel_str (context, "trigger", (class_rmcios) trigger_class_func,
                       0);
   create_channel_str (context, "version", (class_rmcios) version_class_func,
                       0);
   create_channel_str (context, "float", (class_rmcios) float_class_func, 0);
   create_channel_str (context, "int", (class_rmcios) int_class_func, 0);
   create_channel_str (context, "string", (class_rmcios) string_class_func, 0);
   create_channel_str (context, "chain", (class_rmcios) chain_class_func, 0);
}
