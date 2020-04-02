/*  
Use ALSA and soundcard to sample a signal from a signal source and save  it with as .raw file
 * */

// compile with :  gcc -o rekam rekam.c -lasound -lrt -lm -ldl 
// RUN FIX: aplay rekam.c -t raw -c 2 -f S16_LE -r 4800 -D hw:1,0
// run with : ./rekam hw:1,0
#include <stdio.h> 
#include <stdlib.h>
#include <string.h> 
#include <alsa/asoundlib.h> 
#include <unistd.h> 
#include <math.h> 
#include <time.h> 


///////////////////////PROTOTYPE FUNCTION/////////////////////////////


int main(int argc, char *argv[]) {
   
  int i,j;
  int err;
  int16_t *buffer; // buffer integer 16bits
  float *buffer_float;
  int16_t *buffer_int16;
  int rtime = 114 ; // 114 detik estimasi jitter transmisi WSPR
  int buffer_frames_48 = rtime*48000; // alokasi buffer sampling 48KHz sesuai jumlah sampel
  int buffer_frames_12 = (rtime*48000)/8; // alokasi buffer sampling 12 KHz
  unsigned int rate = 48000;// sample rate
  char* devcID;// Sound Card ID
  snd_pcm_t *capture_handle;
  snd_pcm_hw_params_t *hw_params;
  snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE; 
  clock_t t0;
  float tdecimasi=0.0;
  int channel;
  unsigned int min,max;
  char namafile[30];
  
  /*******************************************************/

  devcID = malloc(sizeof(char) * 5); 
  if (argc > 1){
    devcID = argv[1];
   
  }
  else {
    devcID = "hw:1,0"; //default soundcard
  }

  //////////////////////////////Sampling part////////////////////////

  /********Membuka peranti dalam mode perekaman*************/

  if ((err = snd_pcm_open(&capture_handle, devcID, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
    fprintf(stderr, "tidak dapat membuka perangkat %s (%s)\n",devcID,snd_strerror(err));
    exit(1);
  } else {fprintf(stdout, "perangkat berhasil dibuka\n");}

  /**************Penambahan struktur parameter********************/

  if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
    fprintf(stderr, "mustahil untuk menambahkan parameter (%s)\n",
	    snd_strerror(err));
    exit(1);
  }   else { fprintf(stdout, "parameter tambahkan\n"); }
  if ((err = snd_pcm_hw_params_any(capture_handle, hw_params)) < 0) {
    fprintf(stderr, "tidak dapat menginisialisasi parameter (%s)\n",
	    snd_strerror(err));
    exit(1);
  }   else { fprintf(stdout, "parameter diinisialisasi\n"); }

  /*******************Konfigurasi tipe akses ****************************/

  if ((err = snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    fprintf(stderr, "tidak dapat mengonfigurasi jenis akses (%s)\n",
	    snd_strerror(err));
    exit(1);
  }   else { fprintf(stdout, "jenis konfigurasi akses!\n"); }

  /********************************Format data****************************/

  if ((err = snd_pcm_hw_params_set_format(capture_handle, hw_params, format)) < 0) {
    fprintf(stderr, "tidak dapat mengatur format (%s)\n",
	    snd_strerror(err));
    exit(1);
  }   else { fprintf(stdout, "format standar\n"); }

  /********************Mengatur frekuensi pengambilan sampel*************/

  if ((err = snd_pcm_hw_params_set_rate_near(capture_handle, hw_params, &rate, 0)) < 0) {
    fprintf(stderr, "tidak mungkin untuk mengatur frekuensi sampling (%s)\n",
	    snd_strerror(err));
    exit(1);
  }   else { fprintf(stdout, "frekuensi sampling disesuaikan: %d (Hz)\n",rate); }

  /********************Setel jumlah saluran (Mono atau stereo)***************/

  err = snd_pcm_hw_params_get_channels_min(hw_params, &min);
  if (err < 0) {
    fprintf(stderr, "cannot get minimum channels count: %s\n",
	    snd_strerror(err));
    snd_pcm_close(capture_handle);
    return 1;
  }
  err = snd_pcm_hw_params_get_channels_max(hw_params, &max);
  if (err < 0) {
    fprintf(stderr, "cannot get maximum channels count: %s\n",
	    snd_strerror(err));
    snd_pcm_close(capture_handle);
    return 1;
  }
  printf("Channels:");
  for (i = min; i <= max; ++i) {
    if (!snd_pcm_hw_params_test_channels(capture_handle, hw_params, i))
      printf(" %u ", i);
  }


  if ((err = snd_pcm_hw_params_set_channels(capture_handle, hw_params, min)) < 0) {
    fprintf(stderr, "tidak mungkin untuk mengatur jumlah saluran (%s)\n",
	    snd_strerror(err));
    exit(1);
  }   else { fprintf(stdout, "jumlah saluran disesuaikan\n"); }

  /////////////////////////////////////////////////////////////////////////

  if ((err = snd_pcm_hw_params(capture_handle, hw_params)) < 0) {
    fprintf(stderr, "mustahil untuk menerapkan parameter (%s)\n",
	    snd_strerror(err));
    exit(1);
  }   else { fprintf(stdout, "Aplikasi Parameter\n"); }
  snd_pcm_hw_params_free(hw_params);// membersihkan struktur parameter
  fprintf(stdout, "parameter dihapus\n"); 

  /************Persiapan sound card untuk pengambilan***************************/

  if ((err = snd_pcm_prepare(capture_handle)) < 0) {
    fprintf(stderr, "mustahil untuk menyiapkan kartu untuk registrasi (%s)\n",
	    snd_strerror(err));
    exit(1);
  }   else { fprintf(stdout, "siap merekam \n"); }
  //alokasi buffer  16bit, seperti spesifikasi alokasi PCM_FORMAT alokasi memori yang cukup untuk registrasi 
   
  buffer = malloc(buffer_frames_48 * (snd_pcm_format_width(format) / 8) * 2);
  //  buffer_int16 = malloc(buffer_frames_48 * snd_pcm_format_width(format) / 8 * 2);
  //  buffer_float = malloc(buffer_frames_48*sizeof(float));
   
  fprintf(stdout, "memori sudah dialokasikan!!\n");
  
  // Menunggu waktu menit genab

  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  while(tm.tm_min%2 == 0){
  printf("sekarang: %d-%d-%d %d:%d:%d\n", tm.tm_year+1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  t = time(NULL);
  tm = *localtime(&t);
  sleep(1);
  system("clear");
  }
  while(tm.tm_min%2 == 1){
  printf("sekarang: %d-%d-%d %d:%d:%d\n", tm.tm_year+1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  t = time(NULL);
  tm = *localtime(&t);
  sleep(1);
  system("clear");
  if(tm.tm_min%2 ==0)
    sprintf(namafile,"%d:%d:%d:%d:%d.raw", tm.tm_year+1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min);
  }

  fprintf(stdout,"sedang merekam data...[%s]\n",namafile);

  //baca dan simpan di buffer data sound card

  if ((err = snd_pcm_readi(capture_handle, buffer, buffer_frames_48)) != buffer_frames_48) {
    fprintf(stderr, "kesalahan setting antarmuka audio (%s)\n", snd_strerror(err));
    exit(1);
  }
  
  t0=clock();

  //  sigma=sigma/(float)(buffer_frames_12-1);
  tdecimasi += (float)(clock()-t0)/CLOCKS_PER_SEC;

  /////////////////////////END SAMPLING PART///////////////////////////


  /////////////////SAVE SOUNDCARD INPUT//////////////////////////

  FILE *f = fopen(namafile, "wb"); if (f == NULL) {
    printf("Kesalahan membuka file!\n");
    exit(1);
  }

  //  fwrite(buffer, 2, buffer_frames_48, f);    /* jika mono */
  fwrite(buffer, 4, buffer_frames_48, f);       /* jika stereo */
  fclose(f); 
  free(buffer);
  snd_pcm_close(capture_handle);
  //  free(devcID);

}
