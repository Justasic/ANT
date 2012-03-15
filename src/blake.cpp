/* Arbitrary Navn Tool -- Blake-512 Light Hashing Algorithm
 *
 * (C) 2011-2012 Azuru
 * Contact us at Dev@Flux-Net.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "flux.h"
#include "log.h"

#define U8TO32(p) \
  (((uint32_t)((p)[0]) << 24) | ((uint32_t)((p)[1]) << 16) | \
   ((uint32_t)((p)[2]) <<  8) | ((uint32_t)((p)[3])      ))
#define U8TO64(p) \
  (((uint64_t)U8TO32(p) << 32) | (uint64_t)U8TO32((p) + 4))
#define U32TO8(p, v) \
    (p)[0] = (uint8_t)((v) >> 24); (p)[1] = (uint8_t)((v) >> 16); \
    (p)[2] = (uint8_t)((v) >>  8); (p)[3] = (uint8_t)((v)      );
#define U64TO8(p, v) \
    U32TO8((p),     (uint32_t)((v) >> 32));	\
    U32TO8((p) + 4, (uint32_t)((v)      ));

typedef struct  {
  uint64_t h[8], s[4], t[2];
  int buflen, nullt;
  uint8_t buf[128];
} state;

const uint8_t sigma[][16] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15 },
    {14,10, 4, 8, 9,15,13, 6, 1,12, 0, 2,11, 7, 5, 3 },
    {11, 8,12, 0, 5, 2,15,13,10,14, 3, 6, 7, 1, 9, 4 },
    { 7, 9, 3, 1,13,12,11,14, 2, 6, 5,10, 4, 0,15, 8 },
    { 9, 0, 5, 7, 2, 4,10,15,14, 1,11,12, 6, 8, 3,13 },
    { 2,12, 6,10, 0,11, 8, 3, 4,13, 7, 5,15,14, 1, 9 },
    {12, 5, 1,15,14,13, 4,10, 0, 7, 6, 3, 9, 2, 8,11 },
    {13,11, 7,14,12, 1, 3, 9, 5, 0,15, 4, 8, 6, 2,10 },
    { 6,15,14, 9,11, 3, 0, 8,12, 2,13, 7, 1, 4,10, 5 },
    {10, 2, 8, 4, 7, 6, 1, 5,15,11, 9,14, 3,12,13 ,0 },
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15 },
    {14,10, 4, 8, 9,15,13, 6, 1,12, 0, 2,11, 7, 5, 3 },
    {11, 8,12, 0, 5, 2,15,13,10,14, 3, 6, 7, 1, 9, 4 },
    { 7, 9, 3, 1,13,12,11,14, 2, 6, 5,10, 4, 0,15, 8 },
    { 9, 0, 5, 7, 2, 4,10,15,14, 1,11,12, 6, 8, 3,13 },
    { 2,12, 6,10, 0,11, 8, 3, 4,13, 7, 5,15,14, 1, 9 }};

const uint64_t cst[16] = {
  0x243F6A8885A308D3ULL,0x13198A2E03707344ULL,0xA4093822299F31D0ULL,0x082EFA98EC4E6C89ULL,
  0x452821E638D01377ULL,0xBE5466CF34E90C6CULL,0xC0AC29B7C97C50DDULL,0x3F84D5B5B5470917ULL,
  0x9216D5D98979FB1BULL,0xD1310BA698DFB5ACULL,0x2FFD72DBD01ADFB7ULL,0xB8E1AFED6A267E96ULL,
  0xBA7C9045F12C7F99ULL,0x24A19947B3916CF7ULL,0x0801F2E2858EFC16ULL,0x636920D871574E69ULL
};

static const uint8_t padding[129] =
  { 0x80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};


void blake512_compress( state * S, const uint8_t * block ) {

  uint64_t v[16], m[16], i;
#define ROT(x,n) (((x)<<(64-n))|( (x)>>(n)))
#define G(a,b,c,d,e)					\
  v[a] += (m[sigma[i][e]] ^ cst[sigma[i][e+1]]) + v[b];	\
  v[d] = ROT( v[d] ^ v[a],32);				\
  v[c] += v[d];						\
  v[b] = ROT( v[b] ^ v[c],25);				\
  v[a] += (m[sigma[i][e+1]] ^ cst[sigma[i][e]])+v[b];	\
  v[d] = ROT( v[d] ^ v[a],16);				\
  v[c] += v[d];						\
  v[b] = ROT( v[b] ^ v[c],11);

  for(i=0; i<16;++i)  m[i] = U8TO64(block + i*8);
  for(i=0; i< 8;++i)  v[i] = S->h[i];
  v[ 8] = S->s[0] ^ 0x243F6A8885A308D3ULL;
  v[ 9] = S->s[1] ^ 0x13198A2E03707344ULL;
  v[10] = S->s[2] ^ 0xA4093822299F31D0ULL;
  v[11] = S->s[3] ^ 0x082EFA98EC4E6C89ULL;
  v[12] =  0x452821E638D01377ULL;
  v[13] =  0xBE5466CF34E90C6CULL;
  v[14] =  0xC0AC29B7C97C50DDULL;
  v[15] =  0x3F84D5B5B5470917ULL;
  if (S->nullt == 0) {
    v[12] ^= S->t[0];
    v[13] ^= S->t[0];
    v[14] ^= S->t[1];
    v[15] ^= S->t[1];
  }

  for(i=0; i<16; ++i) {
    G( 0, 4, 8,12, 0);
    G( 1, 5, 9,13, 2);
    G( 2, 6,10,14, 4);
    G( 3, 7,11,15, 6);
    G( 3, 4, 9,14,14);
    G( 2, 7, 8,13,12);
    G( 0, 5,10,15, 8);
    G( 1, 6,11,12,10);
  }

  for(i=0; i<16;++i)  S->h[i%8] ^= v[i];
  for(i=0; i<8 ;++i)  S->h[i] ^= S->s[i%4];
}


void blake512_init( state * S ) {

  S->h[0]=0x6A09E667F3BCC908ULL;
  S->h[1]=0xBB67AE8584CAA73BULL;
  S->h[2]=0x3C6EF372FE94F82BULL;
  S->h[3]=0xA54FF53A5F1D36F1ULL;
  S->h[4]=0x510E527FADE682D1ULL;
  S->h[5]=0x9B05688C2B3E6C1FULL;
  S->h[6]=0x1F83D9ABFB41BD6BULL;
  S->h[7]=0x5BE0CD19137E2179ULL;
  S->t[0]=S->t[1]=S->buflen=S->nullt=0;
  S->s[0]=S->s[1]=S->s[2]=S->s[3] =0;

}


void blake512_update( state * S, const uint8_t * data, uint64_t datalen ) {


  int left = (S->buflen >> 3);
  int fill = 128 - left;

  if( left && ( ((datalen >> 3) & 0x7F) >= (unsigned)fill ) ) {
    memcpy( (void *) (S->buf + left), (void *) data, fill );
    S->t[0] += 1024;
    blake512_compress( S, S->buf );
    data += fill;
    datalen  -= (fill << 3);
    left = 0;
  }

  while( datalen >= 1024 ) {
    S->t[0] += 1024;
    blake512_compress( S, data );
    data += 128;
    datalen  -= 1024;
  }

  if( datalen > 0 ) {
    memcpy( (void *) (S->buf + left), (void *) data, ( datalen>>3 ) & 0x7F );
    S->buflen = (left<<3) + datalen;
  }
  else S->buflen=0;
}


void blake512_final( state * S, uint8_t * digest ) {

  uint8_t msglen[16], zo=0x01,oo=0x81;
  uint64_t lo=S->t[0] + S->buflen, hi = S->t[1];
  if ( lo < (unsigned)S->buflen ) hi++;
  U64TO8(  msglen + 0, hi );
  U64TO8(  msglen + 8, lo );

  if ( S->buflen == 888 ) { /* one padding byte */
    S->t[0] -= 8;
    blake512_update( S, &oo, 8 );
  }
  else {
    if ( S->buflen < 888 ) { /* enough space to fill the block */
      if ( S->buflen == 0 ) S->nullt=1;
      S->t[0] -= 888 - S->buflen;
      blake512_update( S, padding, 888 - S->buflen );
    }
    else { /* NOT enough space, need 2 compressions */
      S->t[0] -= 1024 - S->buflen;
      blake512_update( S, padding, 1024 - S->buflen );
      S->t[0] -= 888;
      blake512_update( S, padding+1, 888 );
      S->nullt = 1;
    }
    blake512_update( S, &zo, 8 );
    S->t[0] -= 8;
  }
  S->t[0] -= 128;
  blake512_update( S, msglen, 128 );

  U64TO8( digest + 0, S->h[0]);
  U64TO8( digest + 8, S->h[1]);
  U64TO8( digest +16, S->h[2]);
  U64TO8( digest +24, S->h[3]);
  U64TO8( digest +32, S->h[4]);
  U64TO8( digest +40, S->h[5]);
  U64TO8( digest +48, S->h[6]);
  U64TO8( digest +56, S->h[7]);
}


void blake512_hash( uint8_t *out, const uint8_t *in, uint64_t inlen ) {

  state S;
  blake512_init( &S );
  blake512_update( &S, in, inlen*8 );
  blake512_final( &S, out );
}

bool BlakeHash(Flux::string &rethash, const Flux::string &text, const Flux::string &cmphash)
{

  if(cmphash.size() > 64)
    {
      Log(LOG_TERMINAL) << "[Blake-512]: Comparison hash overflow? is this hash valid?";
      Log(LOG_TERMINAL) << "[Blake-512]: Hash: " << cmphash; 
      return false;
    }
  
  
  uint8_t digest[64];
  uint8_t testdigest[64];
  uint8_t data[65535]; // does it need to be this big and can it be this big???
  bool error = false;
  Flux::string ret;

  for(int i = 0; i < 64; ++i) // clear digest of all data.
    digest[i] = 0;

  if(sizeof(data) < text.size()) // idk??? make sure we don't buffer overflow???
  {
    Log(LOG_TERMINAL) << "[Blake-512]: Text.size() bufferoverflow?";
    Log(LOG_TERMINAL) << "[Blake-512]: data array: " << sizeof(data) << " text: " << text.size();
    return false;
  }
  
  // Convert the Flux::string to a uint8_t array
  for(int i = 0; i < text.size(); ++i)
    data[i] = static_cast<uint8_t>(text[i]);

  // Hash :)
  blake512_hash(digest, data, static_cast<uint64_t>(text.size()));

  // Check if the comparison hash isnt empty, then make comparison on it
  if(!cmphash.empty())
  {
    // move the cmphash into the testdigest array
    for(int i = 0; i < cmphash.size() && i < 64; ++i)
      testdigest[i] = static_cast<uint8_t>(cmphash[i]);

    // compare.
    for(int i = 0; i < 64; ++i)
    {
      if(digest[i] != testdigest[i])
	error = true;
    }
  }

  // put the generated hash into a Flux::string.
  for(int i = 0; i < 64; ++i)
    ret += printfify("%02X", digest[i]);

  rethash = ret;

  if(error)
  {
    Log(LOG_TERMINAL) << "[Blake-512]: Hashes do not match!";
    Log(LOG_TERMINAL) << "[Blake-512]: Hash 1: " << cmphash;
    Log(LOG_TERMINAL) << "[Blake-512]: Hash 2: " << ret;
    return false;
  }
  else
    return true;
  return false;
}
