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
 * Generic measurement device class implementation
 *
 * Changelog: (date,who,description)
 */
#include "RMCIOS-functions.h"

static const char base64_encoding_table[] =
   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// encode to base64. returns the base 64 written bytes. 
// the 4th byte will be placed in reg. 
// 2 MSB of *reg containg the writing state.
char base64_encode (unsigned char *reg, unsigned char c)
{
   char data;
   switch ((*reg) >> 6)
   {
   case 0:
      data = base64_encoding_table[c >> 2];
      *reg = ((c & 3) << 4) | (1 << 6);
      break;
   case 1:
      data = base64_encoding_table[((*reg) & 0x3F) | (c >> 4)];
      *reg = ((c & 0x0F) << 2) | (2 << 6);
      break;
   case 2:
      data = base64_encoding_table[((*reg) & 0x3F) | (c >> 6)];
      *reg = (c & 0x3F) | (3 << 6);
      break;
   case 3:
      data = base64_encoding_table[(*reg) & 0x3F];
      *reg = 0;
      break;
   default:
      data = 0;
      break;
   }
   return data;
}

static const char base64_decoding_table[] = {
// +:   
   62,
   -1, -1, -1,
// /:
   63,
// 0-9:
   52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
   -1, -1, -1, -1, -1, -1, -1,
// A-Z: 
   0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
   21, 22, 23, 24, 25,
   -1, -1, -1, -1, -1, -1,
   26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
   44, 45, 46, 47, 48, 49, 50, 51
};      //+43

// a-z:

int base64_decode (unsigned char *reg, unsigned char c)
{
   //printf("char:%c\n",c) ;
   int rvalue;
   if (c < '+' || c > 'z')
   {
      *reg = 0;
      return -1;
   }
   char index = base64_decoding_table[c - '+'];
   if (index == -1)
   {
      *reg = 0;
      return -1;
   }
   //printf("index:%d\n");


   switch ((*reg) & 3)
   {
   case 0:
      rvalue = -1;
      *reg = (index << 2) | 1;
      break;
   case 1:
      rvalue = (*reg & 0xFC) | (index >> 4);
      *reg = (index << 4) | 2;
      break;
   case 2:
      rvalue = (*reg & 0xFC) | (index >> 2);
      *reg = (index << 6) | 3;
      break;
   case 3:
      rvalue = (*reg & 0xFC) | index;
      *reg = 0;
      break;

   default:
      *reg = 0;
      break;

   }
   return rvalue;
}


struct base64_data
{
   unsigned char state;
};

// Channel to base64 encode binary data
void base64_encoder_class_func (struct base64_data *this,
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
                     "Base64 encoding channel channel "
                     "- channel for encoding binary data\r\n"
                     "create base64_encoder newname\r\n"
                     "write newname data\r\n"
                     "  -Sum the data in buffer form to the checksum\r\n"
                     "write newname\r\n"
                     "  -pad the encoded data and reset encoding machinge.\r\n"
                     "link newname channel\r\n"
                     "  -link encoded output to a channel\r\n");

   case create_rmcios:
      if (num_params < 1)
         break;
      else
      {
         this = (struct base64_data *) 
                allocate_storage (context, sizeof (struct base64_data), 0);
         if (this == 0)
            break;
      }
      this->state = 0;
      // create the channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) base64_encoder_class_func, this); 
      break;

   case write_rmcios:
      if (this == 0)    
      // Run directly without parameters
      {
         if (num_params < 1)
            break;
         unsigned char state = 0;
         int plen = param_buffer_alloc_size (context, paramtype, param, 0);
         {
            char buffer[plen];
            struct buffer_rmcios buf;
            int i;

            buf = param_to_buffer (context, paramtype, param, 0, plen, buffer);

            for (i = 0; i < buf.length; i++)
            {
               char c;
               c = base64_encode (&state, buf.data[i]);
               return_buffer (context, paramtype, returnv, &c, 1);

               if ((state >> 6) == 3)
               {
                  c = base64_encode (&state, 0);
                  return_buffer (context, paramtype, returnv, &c, 1);
               }
            }
            if ((state >> 6) != 0)
            {
               char c;
               c = base64_encode (&state, 0);
               // write the remaining symbol
               return_buffer (context, paramtype, returnv, &c, 1);      
            }
            if ((state >> 6) == 3)
               // pad with "="
               return_string (context, paramtype, returnv, "=");        
            if ((state >> 6) == 2)
               // pad with "=="
               return_string (context, paramtype, returnv, "==");       
         }
         break;
      }
      if (num_params < 1)       
      // PAD & reset
      {
         if ((this->state) >> 6 != 0)
         {
            char c;
            c = base64_encode (&(this->state), 0);
            // write the remaining symbol
            return_buffer (context, paramtype, returnv, &c, 1); 
         }
         if (((this->state) >> 6) == 3)
            // pad with "="
            return_string (context, paramtype, returnv, "=");   
         if (((this->state) >> 6) == 2)
            // pad with "=="
            return_string (context, paramtype, returnv, "==");  
         this->state = 0;
      }
      else      
      // feed data
      {
         int plen = param_buffer_alloc_size (context, paramtype, param, 0);
         {
            char buffer[plen];
            struct buffer_rmcios buf;
            int i;

            buf = param_to_buffer (context, paramtype, param, 0, plen, buffer);

            for (i = 0; i < buf.length; i++)
            {
               char c;
               c = base64_encode (&this->state, buf.data[i]);
               write_buffer (context, linked_channels (context, id), &c, 1, 0);

               if ((this->state >> 6) == 3)
               {
                  c = base64_encode (&(this->state), 0);
                  write_buffer (context,
                                linked_channels (context, id), &c, 1, 0);
               }
            }
         }
      }
      break;
   }
}

// Channel to decode base64 encoded binary data.
void base64_decoder_class_func (struct base64_data *this,
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
                     "Base64 decoding channel channel \r\n"
                     "create base64_decoder newname\r\n"
                     "write newname data\r\n"
                     "  -decode data\r\n"
                     "write newname\r\n"
                     "  -reset the encoding machine.\r\n"
                     "link newname channel\r\n"
                     "  -link decoded output to a channel\r\n");

   case create_rmcios:
      if (num_params < 1)
         break;
      else
      {
         this = (struct base64_data *) 
                allocate_storage (context, sizeof (struct base64_data), 0);
         if (this == 0)
            break;
      }
      this->state = 0;
      // create the channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) base64_decoder_class_func, this);      
      break;

   case write_rmcios:
      if (this == 0)    
      // Run directly without inter-call memory.
      {
         if (num_params < 1)
            break;
         unsigned char state = 0;
         int plen = param_buffer_alloc_size (context, paramtype, param, 0);
         {
            char buffer[plen];
            struct buffer_rmcios buf;
            int i;

            buf = param_to_buffer (context, paramtype, param, 0, plen, buffer);
            for (i = 0; i < buf.length; i++)
            {
               int c;
               char ch;
               c = base64_decode (&state, buf.data[i]);
               ch = c;
               if (c >= 0)
                  return_buffer (context, paramtype, returnv, &ch, 1);
            }
         }
         break;
      }
      if (num_params < 1)       
      // Reset decoding machine
      {
         this->state = 0;
      }
      else      
      // feed data and decode
      {
         int plen = param_buffer_alloc_size (context, paramtype, param, 0);
         {
            char buffer[plen];
            struct buffer_rmcios buf;
            int i;

            buf = param_to_buffer (context, paramtype, param, 0, plen, buffer);
            for (i = 0; i < buf.length; i++)
            {
               int c;
               c = base64_decode (&(this->state), buf.data[i]);
               if (c >= 0)
                  return_buffer (context, paramtype, returnv, (char *) &c, 1);
            }
         }
         break;
      }
      break;
   }
}

//  

const char *hex_encode = "0123456789ABCDEF";

struct hex_data
{
   int datasize;
   char *data;
   int dataindex;
};

void hex_encoder_class_func (struct hex_data *this,
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
                     "Hex encoder. Encodes binary to hex string\r\n"
                     "create hex_encoder newname\r\n"
                     "setup newname bytes(0)\r\n"
                     " -Set ammount bytes to encode. 0=continuous\r\n"
                     "write \r\n"
                     " -Reset byte counter\r\n"
                     "write newname data\r\n"
                     " -feed data to be encoded \r\n"
                     " -returns the encoded string\r\n"
                     " -encoded data go to linked channels\r\n"
                     " -on continuous mode all hex data is sent to linked\r\n"
                     " -otherwise only full set of hex data from n bytes\r\n"
                     "read newname\r\n"
                     " -Get latest hex data. \r\n"
                     " -On continuous mode returns last encoded byte symbol.\r\n"
                     "link newname linked_channel\r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      else
      {
         this = (struct hex_data *) 
                allocate_storage (context, sizeof (struct hex_data), 0);
         if (this == 0)
            break;
      }
      this->data = 0;
      this->datasize = 0;
      this->dataindex = 0;
      // create the channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) hex_encoder_class_func, this); 
      break;
   case setup_rmcios:
      if (this == 0)
         break;
      if (num_params < 1)
         break;

      this->datasize = param_to_int (context, paramtype, param, 0) * 2;
      if (this->data != 0)
         free_storage (context, this->data, 0);
      this->data = allocate_storage (context, this->datasize, 0);
      if (this->data == 0)
         this->datasize = 0;
      break;

   case write_rmcios:
      if (num_params < 1 && this != 0)
      {
         return_buffer (context, paramtype, returnv, this->data,
                        this->dataindex);
         this->dataindex = 0;
         break;
      }
      int plen = param_binary_length (context, paramtype, param, 0);
      {
         char buffer[plen * 2 + 1];
         int i;
         char *data = buffer + plen;
         param_to_binary (context, paramtype, param, 0, plen, data);
         for (i = 0; i < plen; i++)
         {
            char byte = data[i];
            buffer[i << 1] = hex_encode[((byte >> 4) & 0x0F)];
            buffer[(i << 1) + 1] = hex_encode[(byte & 0x0F)];

            if (this != 0 && this->datasize == 0)
            {
               write_buffer (context,
                             linked_channels (context, id),
                             buffer + (i << 1), 2, 0);
            }

            if (this != 0 && this->dataindex < this->datasize - 1)
            {
               this->data[this->dataindex] = buffer[i << 1];
               this->data[this->dataindex + 1] = buffer[(i << 1) + 1];
               this->dataindex += 2;

               if (this->dataindex >= this->datasize - 1)
               {
                  write_buffer (context,
                                linked_channels (context, id),
                                this->data, this->datasize, 0);
               }
            }
         }
         buffer[plen * 2] = 0;
         return_string (context, paramtype, returnv, buffer);
      }
      break;

   case read_rmcios:
      if (this == 0)
         break;
      return_buffer (context, paramtype, returnv, this->data,
                     this->dataindex + 1);
      break;
   }
}

void hex_decoder_class_func (struct hex_data *this,
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
                     "Hex decoder. Decodes hex string to binary.\r\n"
                     "create hex_decoder newname\r\n"
                     "setup newname bytes(0)\r\n"
                     " -Set ammount bytes to decode. 0=continuous\r\n"
                     "write \r\n"
                     " -Reset byte counter\r\n"
                     "write newname data\r\n"
                     " -feed string to be decoded\r\n"
                     " -returns the decoded bytes\r\n"
                     " -decoded data go to linked channels\r\n"
                     " -on continuous mode all decoded data is sent to linked.\r\n"
                     " -otherwise only full set of decoded bytes\r\n"
                     "read newname\r\n"
                     " -Get latest binary data. \r\n"
                     " -On continuous mode returns last decoded byte.\r\n"
                     "link newname linked_channel\r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      else
      {
         this = (struct hex_data *) 
                 allocate_storage (context, sizeof (struct hex_data), 0);
         if (this == 0)
            break;
      }
      this->data = 0 ;
      this->datasize = 0 ;
      this->dataindex = 0 ;
      // create the channel
      create_channel_param (context, paramtype, param, 0, 
                           (class_rmcios) hex_decoder_class_func, this); 
      break;

   case write_rmcios:
      if (num_params < 1 && this != 0)
      {
         return_buffer (context, paramtype, returnv, this->data,
                        this->dataindex);
         this->dataindex = 0;
         break;
      }
      int plen = param_buffer_length (context, paramtype, param, 0);
      {
         char buffer[plen + 1];
         int i;
         char *data = buffer + plen;
         param_to_buffer (context, paramtype, param, 0, plen, data);
         for (i = 0; i < plen; i++)
         {
            char byte = data[i];
            buffer[i << 1] = hex_encode[((byte >> 4) & 0x0F)];

            if (this->datasize == 0)
            {
               write_buffer (context,
                             linked_channels (context, id),
                             buffer + (i << 1), 2, 0);
            }

            if (this != 0 && this->dataindex < this->datasize - 1)
            {
               this->data[this->dataindex] = buffer[i << 1];
               this->data[this->dataindex + 1] = buffer[(i << 1) + 1];
               this->dataindex += 2;

               if (this->dataindex >= this->datasize - 1)
               {
                  write_buffer (context,
                                linked_channels (context, id),
                                this->data, this->datasize, 0);
               }
            }
         }
         buffer[plen] = 0;
         return_string (context, paramtype, returnv, buffer);
      }
      break;

   case read_rmcios:
      if (this == 0)
         break;
      return_buffer (context, paramtype, returnv, this->data,
                     this->dataindex + 1);
      break;
   }
}

const unsigned int bitmask[] =
   { 0, 0x00000001, 0x00000003, 0x00000007, 0x0000000F,
   0x0000001F, 0x0000003F, 0x0000007F, 0x000000FF,
   0x000001FF, 0x000003FF, 0x000007FF, 0x00000FFF,
   0x00001FFF, 0x00003FFF, 0x00007FFF, 0x0000FFFF,
   0x0001FFFF, 0x0003FFFF, 0x0007FFFF, 0x000FFFFF,
   0x001FFFFF, 0x003FFFFF, 0x007FFFFF, 0x00FFFFFF,
   0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF,
   0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF
};

struct binary_decoder_data
{
   // Configuration registers
   int field1_bits;
   int start_offset;
   int repetitions;
   int word_size;
   char endianess;
   int *fields_bits;

   // State registers
   int bit_index;               // Bit offset within message
   int field_index;             // index of field being retreived
   int input_register;
   int latest_field;
};

void binary_decoder_class_func (struct binary_decoder_data *this,
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
            "binary decoder. decodes binary data from binary stream\r\n"
            "create bin_dec newname\r\n"
            "setup newname field1_bits(8) | start_offset(0) | repetitions(0)"
            " | wordsize(8)"
            "  -field1_bits: Bits in the payload value\r\n"
            "  -start_offset: start bit offset of the decoding \r\n"
            "  -repetitions: Ammount of field looping repetitions."
            " 0=continuous\r\n"
            "  -wordsize: size of atomic words in bits\r\n"
            "  -endian: Ordering of words 0=Little-endian(LSB first), "
            "1=Big-Endian(MSB first/Network order)\r\n"
            "  -fields_bits: variable number of optional bitswidths for"
            " fields that follow after the first one.\r\n"
            "write newname\r\n"
            "  -reset field & bit counter\r\n"
            "write newname data\r\n"
            "  -feed data to the decoder\r\n"
            "read newname\r\n"
            "  -read last decoded field\r\n"
            "link newname channel\r\n"
            "  -link converted values to channel\r\n"
        );
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      else
      {
         this = (struct binary_decoder_data *) 
                 allocate_storage (context,
                                   sizeof(struct binary_decoder_data), 0);
         if (this == 0)
            break;
      }

      // Configuration registers
      this->field1_bits = 8;
      this->start_offset = 0;
      this->repetitions = 0;
      this->word_size = 0;
      this->endianess = 1;
      this->fields_bits = 0;

      // State registers
      this->bit_index = 0;      // Bit offset within message
      this->field_index = 0;    // index of field being retreived
      this->latest_field = 0;
      this->input_register = 0;

      // create the channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) binary_decoder_class_func, this);
      break;

   case setup_rmcios:
      if (this == 0)
         break;
      // Configuration registers
      if (num_params < 1)
         break;
      this->field1_bits = param_to_integer (context, paramtype, param, 0);
      if (num_params < 2)
         break;
      this->start_offset = param_to_integer (context, paramtype, param, 1);
      if (num_params < 3)
         break;
      this->repetitions = param_to_integer (context, paramtype, param, 2);
      if (num_params < 4)
         break;
      this->word_size = param_to_integer (context, paramtype, param, 3);
      if (num_params < 5)
         break;
      this->endianess = param_to_integer (context, paramtype, param, 4);
      //if(num_params<6) break ;
      //this->fields_bits=param_to_integer(paramtype,param,5)
      break;
   case write_rmcios:
      if (this == 0)
         break;
      if (num_params == 0)
      {
         this->bit_index = 0;
         this->field_index = 0;
         break;
      }
      else
      {
         int blen;
         int i;
         struct buffer_rmcios p;
         blen = param_binary_length (context, paramtype, param, 0);
         {
            char buffer[blen];
            p = param_to_binary (context, paramtype, param, 0, blen, buffer);
            for (i = 0; i < p.length; i++)
            {
               if (this->endianess == 1)        // Big endian
               {
                  // Big endian
                  this->input_register = this->input_register << 8 | p.data[i];
                  this->bit_index += 8;

                  while (this->bit_index >= this->field1_bits)
                  {
                     this->latest_field =
                        this->input_register >> (this->
                                                 bit_index -
                                                 this->
                                                 field1_bits)
                        & bitmask[this->field1_bits];
                     this->bit_index = this->bit_index - this->field1_bits;
                     write_i (context,
                              linked_channels (context, id),
                              this->latest_field);
                  }
               }
            }
         }
      }
      break;

   case read_rmcios:
      if (this == 0)
         break;
      return_int (context, paramtype, returnv, this->latest_field);
      break;
   }
}


struct binary_encoder_data
{
   // Configuration registers
   int field1_bits;
   int word_size;
   char endianess;
   int num_fields;
   int *fields_bits;

   // State registers
   int bit_index;
   int field_index;             // index of field being sent
   int out_register;
   int latest_word;
};

void binary_encoder_class_func (struct binary_encoder_data *this,
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
                     "binary encoder. Encodes numbers to binary stream\r\n"
                     "create bin_enc newname\r\n"
                     "setup newname field1_bits(8) | wordsize(8) | endian(1)"
                     " | field2_bits field3_bits...\r\n"
                     "write newname\r\n"
                     "write newname value\r\n"
                     "  -write value to data stream\r\n"
                     "link newname channel\r\n"
                     "  -link encoded data to channel\r\n");
      break;
   case create_rmcios:
      if (num_params < 1)
         break;
      else
      {
         this = (struct binary_encoder_data *) 
                allocate_storage 
                (context, sizeof (struct binary_encoder_data), 0);
         if (this == 0)
            break;
      }

      // Configuration registers
      this->field1_bits = 8;
      this->word_size = 8;
      this->endianess = 1;
      this->num_fields = 0;
      this->fields_bits = 0;

      // State registers
      this->bit_index = 0;
      this->field_index = 0;    // index of field being retreived
      this->latest_word = 0;
      this->out_register = 0;

      // Create the channel
      create_channel_param (context, paramtype, param, 0,
                            (class_rmcios) binary_encoder_class_func, this);
      break;

   case setup_rmcios:
      if (this == 0)
         break;
      // Configuration registers
      if (num_params < 1)
         break;
      this->field1_bits = param_to_integer (context, paramtype, param, 0);
      if (num_params < 2)
         break;
      this->word_size = param_to_integer (context, paramtype, param, 1);
      if (num_params < 3)
         break;
      this->endianess = param_to_integer (context, paramtype, param, 2);
      break;

   case write_rmcios:
      if (this == 0)
         break;
      if (num_params == 0)
      {
         this->bit_index = 0;
         this->field_index = 0;
         this->out_register = 0;
      }
      else
      {
         int i, value;
         for (i = 0; i < num_params; i++)
         {
            value = param_to_integer (context, paramtype, param, i);

            // Big endian :
            if (this->endianess == 1)
            {
               this->out_register =
                  (this->out_register << this->
                   field1_bits) | (value & bitmask[this->field1_bits]);
               this->bit_index += this->field1_bits;
               while (this->bit_index >= this->word_size)
               {
                  this->latest_word =
                     this->out_register >> (this->bit_index - this->word_size);
                  this->bit_index -= this->word_size;
                  this->out_register &= bitmask[this->bit_index];
                  write_buffer (context,
                                linked_channels (context, id),
                                (char *) &this->latest_word,
                                this->word_size >> 3, 0);
               }
            }
         }
      }
      break;

   case read_rmcios:
      if (this == 0)
         break;
      return_int (context, paramtype, returnv, this->latest_word);
      break;
   }
}

// function for dynamically loading the module
void init_encoding_channels (const struct context_rmcios *context)
{
   create_channel_str (context, "base64_encoder",
                       (class_rmcios) base64_encoder_class_func, 0);
   create_channel_str (context, "base64_decoder",
                       (class_rmcios) base64_decoder_class_func, 0);
   create_channel_str (context, "hex_enc",
                       (class_rmcios) hex_encoder_class_func, 0);
   create_channel_str (context, "hex_dec",
                       (class_rmcios) hex_decoder_class_func, 0);
   create_channel_str (context, "bin_enc",
                       (class_rmcios) binary_encoder_class_func, 0);
   create_channel_str (context, "bin_dec",
                       (class_rmcios) binary_decoder_class_func, 0);
}

