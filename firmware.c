#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stdlib.h>

FILE* gLogFile = NULL;

typedef enum __tag_LogLevel {
  LL_Emergency,
  LL_Error,
  LL_Warning,
  LL_Information,
  LL_Debug
} LogLevel;

#define ELEMENTSOF(x) (sizeof(x)/sizeof((x)[0]))

#define LOG_FATAL(msgPat, ...) WRITE_LOG(LL_Emergency, msgPat, ##__VA_ARGS__)
#define LOG_ERROR(msgPat, ...) WRITE_LOG(LL_Error, msgPat, ##__VA_ARGS__)
#define LOG_WARNING(msgPat, ...) WRITE_LOG(LL_Warning, msgPat, ##__VA_ARGS__)
#define LOG_INFO(msgPat, ...) WRITE_LOG(LL_Information, msgPat, ##__VA_ARGS__)
#define LOG_DEBUG(msgPat, ...) WRITE_LOG(LL_Debug, msgPat, ##__VA_ARGS__)

  
#define WRITE_LOG(Cat, MsgPat, ...) do { \
    char s[1024] = {0}; \
    sprintf(s, "file{%s}:func{%s}:line{%d}: ", __FILE__, __FUNCTION__, __LINE__); \
    sprintf(s + strlen(s), MsgPat, ##__VA_ARGS__); \
    writeLog(Cat, s); \
  } while (0)
  
#define LOG_PRINT(msg) if (gLogFile) fputs(msg, gLogFile); else puts(msg)

void writeLog(LogLevel cat, const char* msg) {
  switch (cat) {
    case LL_Emergency:
      LOG_PRINT("EMERG: ");
      break;
    case LL_Error:
      LOG_PRINT("ERROR: ");
      break;
    case LL_Warning:
      LOG_PRINT("WARN: ");
      break;
    case LL_Information:
      LOG_PRINT("INFO: ");
      break;
    case LL_Debug:
      LOG_PRINT("DEBUG: ");
      break;
    default:
      LOG_PRINT("UNKNOWN: ");
      break;
  }
  
  LOG_PRINT(msg);
  LOG_PRINT("\n");
}

static bool set_loading(char *loadpath, const char *state)
{
  FILE *ldfile;

  ldfile = fopen(loadpath, "we");
  if (ldfile == NULL) {
    LOG_ERROR("error: can not open '%s'\n", loadpath);
    return false;
  };

  fprintf(ldfile, "%s\n", state);
  fclose(ldfile);
  return true;
}

static bool copy_firmware(const char *source, const char *target, size_t size)
{
  char *buf = NULL;
  FILE *fsource = NULL, *ftarget = NULL;
  bool ret = false;

  buf = malloc(size);
  if (buf == NULL) {
    LOG_ERROR("No memory available to load firmware file");
    return false;
  }

  LOG_DEBUG("writing '%s' (%zi) to '%s'\n", source, size, target);

  fsource = fopen(source, "re");
  if (fsource == NULL) {
    LOG_ERROR("Unable to open source: %s", source);
    goto exit;
  }
  
  ftarget = fopen(target, "we");
  if (ftarget == NULL) {
    LOG_ERROR("Unable to open target: %s", target);
    goto exit;
  }
  
  if (fread(buf, size, 1, fsource) != 1) {
    LOG_ERROR("Fail to read from source");
    goto exit;
  }
  
  if (fwrite(buf, size, 1, ftarget) == 1) {
    ret = true;
  }
  else {
    LOG_ERROR("Fail to write to target");
  }
exit:
  if (ftarget != NULL) {
    fclose(ftarget);
  }
  if (fsource != NULL) {
    fclose(fsource);
  }
  
  free(buf);
  return ret;
}


static int builtin_firmware(const char* firmware, const char* devpath)
{
  static const char *searchpath[] = { "/lib/firmware", "/lib/firmware/update" };
  char loadpath[1024] = { 0 };
  char datapath[1024] = { 0 };
  char fwpath[1024] = { 0 };
  FILE *fwfile = NULL;
  struct stat statbuf;
  unsigned int i = 0;
  int rc = 0;

  /* lookup firmware file */
  for (i = 0; i < ELEMENTSOF(searchpath); i++) {
    sprintf(fwpath, "%s/%s", searchpath[i], firmware);
    fwfile = fopen(fwpath, "re");
    if (fwfile != NULL) {
      break;
    }
  }

  sprintf(loadpath, "%s/%s", devpath, "loading");

  if (fwfile == NULL) {
    LOG_ERROR("did not find firmware file '%s'\n", firmware);
    rc = -1;
    
    set_loading(loadpath, "-1");
    goto exit;
  }

  if (stat(fwpath, &statbuf) < 0 || statbuf.st_size == 0) {
    LOG_ERROR("firmware file stat wrong '%s'\n", firmware);
    set_loading(loadpath, "-1");
    rc = -1;
    goto exit;
  }

  if (!set_loading(loadpath, "1")) {
    LOG_ERROR("fail to set loading status to 1");
    rc = -1;
    goto exit;
  }

  sprintf(datapath, "%s/%s", devpath, "data");
  if (!copy_firmware(fwpath, datapath, statbuf.st_size)) {
    LOG_ERROR("error sending firmware '%s' to device\n", firmware);
    set_loading(loadpath, "-1");
    rc = -1;
    goto exit;
  };

  set_loading(loadpath, "0");
exit:
  if (fwfile) {
    fclose(fwfile);
  }
  return rc;
}

int main(int argc, char* argv[]) {
  char* logFileName = NULL;
  char* devPath = NULL;
  char* firmwareName = NULL;

  LOG_INFO("args: %d", argc);
  
  if (argc < 3 || argc > 4) {
    return -1;
  }
  else if (argc == 4) {
    logFileName = argv[3];
    gLogFile = fopen(logFileName, "w");
  }
  
  firmwareName = argv[1];
  devPath = argv[2];
  
  builtin_firmware(firmwareName, devPath);
  
  if (gLogFile) {
    fclose(gLogFile);
    gLogFile = NULL;
  }
  
  return 0;
}