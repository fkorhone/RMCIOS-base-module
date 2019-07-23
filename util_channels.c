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

/////////////////////////////////////////////////
//! Channel for logging data to linked channel //
/////////////////////////////////////////////////
struct logger_data
{
   char reset_char;
   char delimiter_char;
   int *log_channels;
   int num_logged;
   int newline;
};

void logger_class_func (struct logger_data *this,
                        const struct context_rmcios *context, int id,
                        enum function_rmcios function,
                        enum type_rmcios paramtype,
                        struct combo_rmcios *returnv,
                        int num_params, const union param_rmcios param)
{
   int pstrlen;
   switch (function)
   {
   case help_rmcios:
      return_string (context, returnv,
              "help for logger channel\r\n"
              "create logger newname\r\n"
              "setup newname reset_char log_delimiter log_channels... \r\n"
              "write newname # trigger log action."
              " Adds reset char after log channels. \r\n"
              "write newname data # write data to output channel.\r\n"
              "      On reset_char found: "
              "inserts output channels just before next character.\r\n"
              "link newname output_channel "
              " # link logger output to channel\r\n");
      break;
   case create_rmcios:
      if (num_params < 1)
         break;
      // allocate new data
      this = (struct logger_data *) 
             allocate_storage (context, sizeof (struct logger_data), 0); 
      this->reset_char = '\n';
      this->delimiter_char = ' ';
      this->log_channels = 0;
      this->newline = 1;
      this->num_logged = 0;

      // create channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) logger_class_func, this); 
      break;

   case setup_rmcios:
      if (this == 0)
      {
         break;
      }
      if (num_params < 3)
         break;
      {
         int i;
         param_to_buffer (context, paramtype, param, 0, 1, &this->reset_char);
         param_to_buffer (context, paramtype, param, 1, 1,
                          &this->delimiter_char);

         if (this->log_channels != 0)
            free_storage (context, this->log_channels, 0);
         this->num_logged = (num_params - 2);
         this->log_channels =
            (int *) allocate_storage (context,
                                      this->num_logged * sizeof (int), 0);
         for (i = 0; i < this->num_logged; i++)
            this->log_channels[i] =
               param_to_int (context, paramtype, param, i + 2);

      }
      break;

   case write_rmcios:
      if (this == 0)
      {
         break;
      }
      if (num_params < 1)       
      // direct trigger of log action
      {
         char wstr[2] = { 0 };
         int j;
         for (j = 0; j < this->num_logged; j++)
         {
            struct combo_rmcios destination = {
               .paramtype = channel_rmcios,
               .num_params = 1,
               .param.channel = linked_channels (context, id)
            };
            
            run_channel (context, this->log_channels[j],
                                  read_rmcios,
                                  channel_rmcios,
                                  &destination, 0,
                                  (const union param_rmcios) 0);

            if (j < this->num_logged - 1)
            {
               wstr[0] = this->delimiter_char;
               // add delimiter
               write_str (context, linked_channels (context, id), wstr, 0); 
            }
         }
         wstr[0] = this->reset_char;
         // add end of line (reset char) 
         write_str (context, linked_channels (context, id), wstr, 0);   
         break;
      }

      pstrlen = param_string_alloc_size (context, paramtype, param, 0);
      {
         int j;
         char wstr[2] = { 0 };
         char str[pstrlen];
         const char *s;
         s = param_to_string (context, paramtype, param, 0, pstrlen, str);
         while (*s != 0)
         {
            if (*s
                  == 0)
               break;

            if (this->newline == 1 && *s != this->reset_char)   
            // trigger channels
            {
               for (j = 0; j < this->num_logged; j++)
               {
                  struct combo_rmcios destination = {
                     .paramtype = channel_rmcios,
                     .num_params = 1,
                     .param.channel = linked_channels (context, id)
                  };

                  run_channel (context,
                                        this->log_channels[j],
                                        read_rmcios,
                                        channel_rmcios,
                                        &destination, 
                                        0,
                                        (const union param_rmcios) 0);

                  wstr[0] = this->delimiter_char;
                  // add delimiter
                  write_str (context, linked_channels (context, id), wstr, 0);  
               }

               this->newline = 0;
            }
            wstr[0] = *s;

            if (*s == this->reset_char)
            {
               // newline -> next input character will trigger log entry.
               this->newline = 1;       
            }
            write_str (context, linked_channels (context, id), wstr, 0);
            s++;
         }
      }
      break;
   }
}

/////////////////////////////////////////////////////////////
// Channel to calculate two's complement checksums.
/////////////////////////////////////////////////////////////
struct checksum_data
{
   unsigned char checksum;
};

// Should later be upgraded to handle multiple byte checksums with variying byte orderings.
void checksum_class_func (struct checksum_data *this,
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
                     "Checksum channel -"
                     " channel for creating and checking 8bit two's complement"
                     " modular checksums.\r\n"
                     "create checksum newname\r\n"
                     "setup newname sum\r\n"
                     "  -Set the stored sum\r\n"
                     "write newname data\r\n"
                     "  -Sum the data in buffer form to the checksum\r\n"
                     "write newname\r\n"
                     "  -Sends checksum in inverted form to linked channels.\r\n"
                     "  -Set the stored sum to 0\r\n"
                     "read newname\r\n"
                     "  -Returns stored checksum\r\n"
                     "  -Tests the stored checksum for 0."
                     " If true makes empty write to linked channels.\r\n"
                     "link newname channel\r\n");

   case create_rmcios:
      if (num_params < 1)
         break;
      // allocate new data
      this = (struct checksum_data *) 
             allocate_storage (context, sizeof (struct checksum_data), 0);     

      if (this == 0)
         break;
      this->checksum = 0;

      // create the channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) checksum_class_func, this);
      break;

   case setup_rmcios:
      if (this == 0)
         break;
      if (num_params < 0)
         break;
      this->checksum = param_to_int (context, paramtype, param, 0);
      break;

   case write_rmcios:
      if (this == 0)
         break;
      if (num_params < 1)       
      // Send checksum in two's complement to linked channels
      {
         write_i (context, linked_channels (context, id),
                  (~this->checksum) + 1);
         this->checksum = 0;
      }
      else      
      // Sum the parameter in buffer form.
      {
         int plen = param_buffer_alloc_size (context, paramtype, param, 0);
         // Determine the size of possibly needed buffer
         {
            // Allocate the possibly needed buffer
            char buffer[plen];  
            // Structure for handling the parameter data in buffer form.
            struct buffer_rmcios buf;   
            int i;

            // Get the struct pointing to the parameter buffer:
            buf = param_to_buffer (context, paramtype, param, 0, plen, buffer);

            // Sum the bytes
            for (i = 0; i < buf.length; i++)
            {
               this->checksum += (unsigned char) buf.data[i];
            }
         }
      }
      break;

   case read_rmcios:
      if (this == 0)
         break;
      // Return the stored checksum
      return_int (context, returnv, this->checksum);
      // Send reset command (empty write) to linked channels
      // when checksum is correct(0) 
      if (this->checksum == 0)
         write_fv (context, linked_channels (context, id), 0, 0);
      break;
   }
}

//////////////////////////////////////////////////////////////////////////////
// LCG channel - Linear congruential generator
//////////////////////////////////////////////////////////////////////////////

struct lcg_data
{
   int a;
   int c;
   int seed;
   int modulus;
   float scale;
};

void lcg_random_class_func (struct lcg_data *this,
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
                     "LCG channel -"
                     " Linear congruential random number generator \r\n"
                     " seed = (a * seed + c) % modulus -> out=seed*scale \r\n"
                     "create lcg newname \r\n"
                     "setup newname seed(0) | a(2) | c(1) |"
                     " modulus(8) | scale(1) \r\n"
                     "write newname # Calculate new seed,"
                     " write to linked channels\r\n"
                     "read newname # Read latest seed\r\n"
                     "link newname linked_channel\r\n");
      break;
   case create_rmcios:
      if (num_params < 1)
         break;
      // allocate new data
      this = (struct lcg_data *) 
             allocate_storage (context, sizeof (struct lcg_data), 0);

      if (this == 0)
         break;
      this->a = 2;
      this->c = 1;
      this->seed = 0;
      this->modulus = 8;
      this->scale = 1;

      // create the channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) lcg_random_class_func, this);  
      break;
   case setup_rmcios:
      if (this == 0)
         break;
      if (num_params < 1)
         break;
      this->seed = param_to_int (context, paramtype, param, 0);
      if (num_params < 2)
         break;
      this->a = param_to_int (context, paramtype, param, 1);
      if (num_params < 3)
         break;
      this->c = param_to_int (context, paramtype, param, 2);
      if (num_params < 4)
         break;
      this->modulus = param_to_int (context, paramtype, param, 3);
      if (num_params < 5)
         break;
      this->scale = param_to_float (context, paramtype, param, 4);
      break;

   case write_rmcios:
      if (this == 0)
         break;
      this->seed = ((this->a * this->seed + this->c) % this->modulus);
      write_f (context, linked_channels (context, id),
               this->scale * this->seed);
      break;

   case read_rmcios:
      return_float (context, returnv, this->scale * this->seed);
      break;
   }
}

//////////////////////////////////////////////////////
// Filter channel
//////////////////////////////////////////////////////
struct filter_data
{
   float value;
   int test;                    // ascii char1 | cha2
};

void filter_class_func (struct filter_data *this,
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
                     "filter channel\r\n"
                     "create filter newname\r\n"
                     "setup newname value | test\r\n"
                     " test can be one of following:\r\n"
                     " = # filter value\r\n"
                     " == # filter value\r\n"
                     " > # filter greater than value\r\n"
                     " < # filter less than value\r\n"
                     " <= # filter less or equal of value\r\n"
                     " =< # filter less or equal of value\r\n"
                     " >= # filter greater or equal than value\r\n"
                     " => # filter greater or equal than value\r\n"
                     " =0 # filter all when value==0\r\n"
                     " =1 # filter all when value==1\r\n"
                     " 0 # pass all when value==0\r\n"
                     " 1 # pass all when value==1\r\n"
                     "write newname value\r\n"
                     " -write value to filter and pass nonfiltered" 
                     " to linked channels\r\n"
                     "link newname channel\r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      // allocate new data
      this = (struct filter_data *) 
             allocate_storage (context, sizeof (struct filter_data), 0); 
      if (this == 0)
         break;
      this->test = 0;
      // create the channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) filter_class_func, this);
      break;

   case setup_rmcios:
      if (this == 0)
         break;
      if (num_params < 1)
         break;
      this->value = param_to_float (context, paramtype, param, 0);
      if (num_params > 1)
      {
         char test[2];
         test[1] = 0;
         param_to_buffer (context, paramtype, param, 1, 2, test);
         this->test = test[0] + test[1];
      }
      break;

   case write_rmcios:
      if (this == 0)
         break;
      if (num_params > 0)
      {
         float value = param_to_float (context, paramtype, param, 0);
         switch (this->test)
         {
         case '=':
            if (this->value == value)
               return;
            break;

         case '<' + '=':
            if (this->value == value)
               return;
         case '<':
            if (this->value < value)
               return;
            break;

         case '>' + '=':
            if (this->value == value)
               return;
         case '>':
            if (this->value > value)
               return;
            break;
         case '0' + '0':
            if (this->value != 0)
               return;
         case '1':
            if (this->value != 1)
               return;
         default:
            // Undefined filter identifier
            return;     
         }
         run_channel (context, linked_channels (context, id),
                               function, paramtype, returnv, num_params, param);
      }
      break;
   }
}

// based on code of: Emilie Laverge - MBED - MIT licence:
// And A PAINLESS GUIDE TO CRC ERROR DETECTION ALGORITHMS
//Version : 3.
//Date    : 19 August 1993.
//Author  : Ross N. Williams.

static const unsigned char BitReverseTable256[] = {
   0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0,
   0x30, 0xB0, 0x70, 0xF0,
   0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8,
   0x38, 0xB8, 0x78, 0xF8,
   0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4,
   0x34, 0xB4, 0x74, 0xF4,
   0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC,
   0x3C, 0xBC, 0x7C, 0xFC,
   0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2,
   0x32, 0xB2, 0x72, 0xF2,
   0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA,
   0x3A, 0xBA, 0x7A, 0xFA,
   0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6,
   0x36, 0xB6, 0x76, 0xF6,
   0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE,
   0x3E, 0xBE, 0x7E, 0xFE,
   0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1,
   0x31, 0xB1, 0x71, 0xF1,
   0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9,
   0x39, 0xB9, 0x79, 0xF9,
   0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5,
   0x35, 0xB5, 0x75, 0xF5,
   0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD,
   0x3D, 0xBD, 0x7D, 0xFD,
   0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3,
   0x33, 0xB3, 0x73, 0xF3,
   0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB,
   0x3B, 0xBB, 0x7B, 0xFB,
   0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7,
   0x37, 0xB7, 0x77, 0xF7,
   0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF,
   0x3F, 0xBF, 0x7F, 0xFF
};

const unsigned short crc16table_0x8005[256] = {
   0x0000, 0x8005, 0x800F, 0x000A, 0x801B, 0x001E, 0x0014, 0x8011,
   0x8033, 0x0036, 0x003C, 0x8039, 0x0028, 0x802D, 0x8027, 0x0022,
   0x8063, 0x0066, 0x006C, 0x8069, 0x0078, 0x807D, 0x8077, 0x0072,
   0x0050, 0x8055, 0x805F, 0x005A, 0x804B, 0x004E, 0x0044, 0x8041,
   0x80C3, 0x00C6, 0x00CC, 0x80C9, 0x00D8, 0x80DD, 0x80D7, 0x00D2,
   0x00F0, 0x80F5, 0x80FF, 0x00FA, 0x80EB, 0x00EE, 0x00E4, 0x80E1,
   0x00A0, 0x80A5, 0x80AF, 0x00AA, 0x80BB, 0x00BE, 0x00B4, 0x80B1,
   0x8093, 0x0096, 0x009C, 0x8099, 0x0088, 0x808D, 0x8087, 0x0082,
   0x8183, 0x0186, 0x018C, 0x8189, 0x0198, 0x819D, 0x8197, 0x0192,
   0x01B0, 0x81B5, 0x81BF, 0x01BA, 0x81AB, 0x01AE, 0x01A4, 0x81A1,
   0x01E0, 0x81E5, 0x81EF, 0x01EA, 0x81FB, 0x01FE, 0x01F4, 0x81F1,
   0x81D3, 0x01D6, 0x01DC, 0x81D9, 0x01C8, 0x81CD, 0x81C7, 0x01C2,
   0x0140, 0x8145, 0x814F, 0x014A, 0x815B, 0x015E, 0x0154, 0x8151,
   0x8173, 0x0176, 0x017C, 0x8179, 0x0168, 0x816D, 0x8167, 0x0162,
   0x8123, 0x0126, 0x012C, 0x8129, 0x0138, 0x813D, 0x8137, 0x0132,
   0x0110, 0x8115, 0x811F, 0x011A, 0x810B, 0x010E, 0x0104, 0x8101,
   0x8303, 0x0306, 0x030C, 0x8309, 0x0318, 0x831D, 0x8317, 0x0312,
   0x0330, 0x8335, 0x833F, 0x033A, 0x832B, 0x032E, 0x0324, 0x8321,
   0x0360, 0x8365, 0x836F, 0x036A, 0x837B, 0x037E, 0x0374, 0x8371,
   0x8353, 0x0356, 0x035C, 0x8359, 0x0348, 0x834D, 0x8347, 0x0342,
   0x03C0, 0x83C5, 0x83CF, 0x03CA, 0x83DB, 0x03DE, 0x03D4, 0x83D1,
   0x83F3, 0x03F6, 0x03FC, 0x83F9, 0x03E8, 0x83ED, 0x83E7, 0x03E2,
   0x83A3, 0x03A6, 0x03AC, 0x83A9, 0x03B8, 0x83BD, 0x83B7, 0x03B2,
   0x0390, 0x8395, 0x839F, 0x039A, 0x838B, 0x038E, 0x0384, 0x8381,
   0x0280, 0x8285, 0x828F, 0x028A, 0x829B, 0x029E, 0x0294, 0x8291,
   0x82B3, 0x02B6, 0x02BC, 0x82B9, 0x02A8, 0x82AD, 0x82A7, 0x02A2,
   0x82E3, 0x02E6, 0x02EC, 0x82E9, 0x02F8, 0x82FD, 0x82F7, 0x02F2,
   0x02D0, 0x82D5, 0x82DF, 0x02DA, 0x82CB, 0x02CE, 0x02C4, 0x82C1,
   0x8243, 0x0246, 0x024C, 0x8249, 0x0258, 0x825D, 0x8257, 0x0252,
   0x0270, 0x8275, 0x827F, 0x027A, 0x826B, 0x026E, 0x0264, 0x8261,
   0x0220, 0x8225, 0x822F, 0x022A, 0x823B, 0x023E, 0x0234, 0x8231,
   0x8213, 0x0216, 0x021C, 0x8219, 0x0208, 0x820D, 0x8207, 0x0202
};

const unsigned int SHIFTER = 0x00FF;

struct crc16_data
{
   unsigned short CRC16;
   unsigned short poly;
   const unsigned short *crc16table;
   unsigned short init;
   char refin;
   char refout;
   unsigned short xor_out;
};

// Should be upgraded to handle byte checksums with variying byte orderings.
void crc16_class_func (struct crc16_data *this,
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
               "CRC channel - "
               "channel for creating and checking CRC checksums.\r\n"
               "create crc newname | bits(16)\r\n"
               "write crc data\r\n"
               "  -return the crc for data\r\n"
               "setup newname sum(0) | Poly(0x8005) | Init(0) |"
               " RefIn(0) | RefOut(0) | XorOut(0x0000)   \r\n"
               "  -Set the stored sum, and calculation polynomial\r\n"
               "setup newname\r\n"
               "  -Reset the sum\r\n"
               "write newname data\r\n"
               "  -Sum the data in buffer to the checksum\r\n"
               "write newname\r\n"
               "  -Sends checksum to linked channels.\r\n"
               "  -Reset the sum\r\n"
               "read newname\r\n"
               "  -Returns stored checksum\r\n"
               "link newname channel\r\n");
         break;

      case create_rmcios:
         if (num_params < 1)
            break;
         // allocate new data
         this = (struct crc16_data *) 
            allocate_storage (context, sizeof (struct crc16_data), 0);   
         if (this == 0)
            break;
         this->CRC16 = 0;
         this->poly = 0x8005;
         this->crc16table = crc16table_0x8005;
         this->init = 0;
         this->refin = 0;
         this->refout = 0;
         this->xor_out = 0;
         // create the channel
         create_channel_param (context, paramtype, param, 0, 
               (class_rmcios) crc16_class_func, this); 
         break;

      case setup_rmcios:
         if (this == 0)
            break;
         this->CRC16 = this->init;
         if (num_params < 1)
            break;
         this->CRC16 = param_to_integer (context, paramtype, param, 0);
         if (num_params < 2)
            break;
         this->poly = param_to_integer (context, paramtype, param, 1);
         switch (this->poly)
         {
            case 0x8005:
               this->crc16table = crc16table_0x8005;
               break;
            default: // Allocate and calculate new lookup table
               {
                  unsigned short index;
                  unsigned short r;
                  unsigned short *t;
                  t = (unsigned short *) allocate_storage (context,
                        256 *
                        sizeof (unsigned
                           short), 0);
                  for (index = 0; index < 256; index++)
                  {
                     int i;
                     r = index << 8;
                     for (i = 0; i < 8; i++)
                     {
                        if (r & 0x8000)
                           r = (r << 1) ^ this->poly;
                        else
                           r <<= 1;
                     }
                     t[index] = r & 0xFFFF;
                  }
                  this->crc16table = t;
               }
               break;
         }
         if (num_params < 3)
            break;
         this->init = param_to_integer (context, paramtype, param, 2);
         this->CRC16 = this->init;
         if (num_params < 4)
            break;
         this->refin = param_to_integer (context, paramtype, param, 3);
         if (num_params < 5)
            break;
         this->refout = param_to_integer (context, paramtype, param, 4);
         if (num_params < 6)
            break;
         this->xor_out = param_to_integer (context, paramtype, param, 5);
         break;

      case read_rmcios:
         if (this == 0)
            break;

         unsigned short CRC16;
         if (this->refout == 1)
            CRC16 =
               this->
               xor_out ^ (BitReverseTable256[this->CRC16 >> 8] |
                     BitReverseTable256[this->CRC16 & 0xFF] << 8);
         else
            CRC16 = this->xor_out ^ this->CRC16;

         return_int (context, returnv, CRC16);
         break;

      case write_rmcios:
         if (this == 0)    // Default (modbus CRC)
         {
            unsigned short CRC16 = 0;
            int blen = param_buffer_alloc_size (context, paramtype, param, 0);
            {
               char buffer[blen];
               struct buffer_rmcios p;
               int i;
               p = param_to_buffer (context, paramtype, param, 0, blen, buffer);
               char *input = p.data;
               for (i = 0; i < p.length; i++)
               {
                  unsigned char byte = *(unsigned char *) input++;
                  // Reflect input
                  byte = BitReverseTable256[byte];
                  unsigned short tableValue =
                     crc16table_0x8005[((CRC16 >> 8) ^ byte) & SHIFTER];
                  CRC16 = (CRC16 << 8) ^ tableValue;
               }

               // Reflect output
               return_int (context, returnv,
                     BitReverseTable256[CRC16 >> 8] |
                     BitReverseTable256[CRC16 & 0xFF] << 8);
            }
            break;
         }
         if (num_params < 1)       // Calculate and send
         {
            unsigned short CRC16;
            if (this->refout == 1)
               CRC16 =
                  this->
                  xor_out ^ (BitReverseTable256[this->CRC16 >> 8] |
                        BitReverseTable256[this->CRC16 & 0xFF] << 8);
            else
               CRC16 = this->xor_out ^ this->CRC16;

            write_binary (context, linked_channels (context, id),
                  (char *) &CRC16, 2, 0, 0);
            return_int (context, returnv, CRC16);
            this->CRC16 = this->init;
            break;
         }

         int blen = param_buffer_alloc_size (context, paramtype, param, 0);
         {
            char buffer[blen];
            struct buffer_rmcios p;
            int i;
            p = param_to_buffer (context, paramtype, param, 0, blen, buffer);
            char *input = p.data;
            for (i = 0; i < p.length; i++)
            {
               unsigned char byte = *(unsigned char *) input++;
               if (this->refin == 1)
                  byte = BitReverseTable256[byte];
               unsigned short tableValue =
                  this->crc16table[(((this->CRC16) >> 8) ^ byte) & SHIFTER];
               this->CRC16 = ((this->CRC16) << 8) ^ tableValue;
            }
         }
         break;
   }
}


void init_util_channels (const struct context_rmcios *context)
{
   create_channel_str (context, "logger", (class_rmcios) logger_class_func, 0);
   create_channel_str (context, "modsum", (class_rmcios) checksum_class_func,
                       0);
   create_channel_str (context, "lcg", (class_rmcios) lcg_random_class_func, 0);
   create_channel_str (context, "filter", (class_rmcios) filter_class_func, 0);
   create_channel_str (context, "crc", (class_rmcios) crc16_class_func, 0);
}
