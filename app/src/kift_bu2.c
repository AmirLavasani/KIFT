#include "kift.h"

static t_cmd g_cmds[NB_INSTRUCTIONS]=
{
	{0, NULL, NULL, 0},
	{1, "ALARM", "SET ALARM", 0},
	{2, "GVU", "GET WEATHER", 0},
	{3, "KITCHEN", "BRIAN IS IN THE KITCHEN", 1},
	{4, "PLAY", "PLAY MUSIC", 1},
	{5, "CHECK", "CHECK WEATHER", 1},
	{6, "EMAIL", "SEND EMAIL", 1},
	{7, "GOOGLE", "SEARCH ON GOOGLE", 1},
	{8, "GET UP", "I DON'T SET THE ALARM TO GET UP", 1},
	{9, "FEEL", "I GET UP WHEN I FEEL LIKE IT", 1},
	{10, "LOVE", "I LOVE THE RAIN", 1},
	{11, "FAVORITE", "IT'S MY FAVORITE WEATHER", 1},
	{12, "IF", "IF IT ISN'T ON GOOGLE", 1},
	{13, "EXIT", "IT DOESN'T EXIST", 1},
	{14, "WHEREVER", "WHEREVER YOU GO", 1},
	{15, "MATTER", "NO MATTER WHAT THE WEATHER", 1},
	{16, "BRING", "ALWAYS BRING YOUR OWN SUNSHINE", 1}
};

char *toupper_str(char *str)
{
  int i = 0;
  int diff = 'A' - 'a';

  if (!str || !str[0])
    return (str);
  while (str[i])
    {
      if (str[i] >= 'a' && str[i] <= 'z')
	str[i] = str[i] + diff;
      i++;
    }
  return (str);
}


t_cmd *get_cmd_by_hyp(char *hyp)
{
	int i;

	i = 0;
	while (++i < NB_INSTRUCTIONS)
	{
	  if (strstr(toupper_str(hyp), toupper_str(g_cmds[i].key_words)))
			return (g_cmds + i);
	}
	return (g_cmds);
}

int write_logs_response(t_cmd *cmd, char *argv[], char *hyp)
{
	char *user_files_path;
	int fd;

	fd = -1;
	user_files_path = NULL;
	if (!cmd->is_train)
	{
		asprintf(&user_files_path, "%s%s%s", BASE_LOG, argv[2], LOG_FILE);
		if ((fd = open(user_files_path, O_RDWR | O_CREAT | O_APPEND, 0666)) == -1)
			return (-1);
		dprintf(fd, "%s (%s)\n", hyp, argv[1]);
		close(fd);
		asprintf(&user_files_path, "%s%s%s", BASE_LOG, argv[2], RESPONSE_INSTRUCTION_FILE);
		if ((fd = open(user_files_path, O_RDWR | O_CREAT , 0666)) == -1)
			return (-1);
		dprintf(fd, "%i", cmd->id);
		close(fd);
		asprintf(&user_files_path,"%s%s%s", BASE_LOG, argv[2], RESPONSE_TRAIN_FILE);
		remove(user_files_path);
	}
	else
	{
		asprintf(&user_files_path, "%s%s%s", BASE_LOG, argv[2], RESPONSE_TRAIN_FILE);
		if ((fd = open(user_files_path, O_RDWR | O_CREAT, 0666)) == -1)
			return (-1);
		if (cmd->id + 1 < NB_INSTRUCTIONS)
			dprintf(fd, "%s\n", (cmd + 1)->train_sentence);
		else
			dprintf(fd, "%s\n", END_OF_TRAIN);
		write(fd, "\0", 1);
		close(fd);
		asprintf(&user_files_path, "%s%s%s", BASE_LOG, argv[2], RESPONSE_INSTRUCTION_FILE);
		remove(user_files_path);
	}
	return (1);
}

int write_for_train(t_cmd *cmd, char *argv[])
{
	int fd;
	char *path;

	fd = -1;
	asprintf(&path, "%s%s%s", BASE_TRAIN_UTILS, argv[2], TRANSCRIPTION_FILE);
	if ((fd = open(path, O_RDWR | O_CREAT | O_APPEND, 0666)) == -1)
		return (-1);
	dprintf(fd, "<s> %s </s> (%s)\n", cmd->train_sentence, argv[1]);
	close(fd);
	asprintf(&path, "%s%s%s", BASE_TRAIN_UTILS, argv[2], FILEIDS_FILE);
	if ((fd = open(path, O_RDWR | O_CREAT | O_APPEND, 0666)) == -1)
		return (-1);
	dprintf(fd, "%s\n", argv[1]);
	close(fd);
	return (1);
}

int update_model(char *username)
{
	char *path;

	asprintf(&path, "%s %s", TRAIN_SH_FILE, username);
	return (system(path));
}

const char *get_hyp(char *argv[])
{
  ps_decoder_t *ps;
  cmd_ln_t *config;
  char const *hyp, *uttid;
  int16 buf[512];
  int rv;
  int32 score;
  FILE *fh;

  //      config = cmd_ln_init(NULL, ps_args(), TRUE,
  //                      "-hmm", MODELDIR "/en-us/en-us",
  //                      "-lm","/var/www/html/app/tutorial/init.lm",
  //                      "-dict", "/var/www/html/app/tutorial/init.dic",
  //                      NULL);
  config = cmd_ln_init(NULL, ps_args(), TRUE,
		       "-hmm", "/var/www/html/app/tutorial/en-us-adapt",
//		                             "-hmm", MODELDIR "/en-us/en-us",
		       "-lm",  "/var/www/html/app/tutorial/en-us.lm.bin",
		       "-jsgf", "/var/www/html/app/src/hello_team.gram",
		       "-dict", "/var/www/html/app/tutorial/cmudict-en-us.dict",
		       "-kws", "/var/www/html/app/tutorial/keyphrase.list",
		       NULL);
  if (config == NULL) {
    fprintf(stderr, "Failed to create config object, see log for  details\n");
    return NULL;
  }
  ps = ps_init(config);
  if (ps == NULL) {
    fprintf(stderr, "Failed to create recognizer, see log for  details\n");
    return NULL;
  }
  char *tmp;
  asprintf(&tmp, "%s%s/%s", BASE_AUDIO, argv[2], argv[1]);
  //printf("\n\nwhich AUDIO >>>>>>>>>> %s\n\n", tmp);
  fh = fopen(tmp, "rb");
  if (fh == NULL) {
    fprintf(stderr, "Unable to open input file %s\n", argv[1]);
    return NULL;
  }
  rv = ps_start_utt(ps);
  char *username;
  username = argv[2];
  if (username == NULL)
    {
      fprintf(stderr, "Failed to assign username\n");
      return (NULL);
    }

  while (!feof(fh)) {
    size_t nsamp;
    nsamp = fread(buf, 2, 512, fh);
    rv = ps_process_raw(ps, buf, nsamp, FALSE, FALSE);
  }

  rv = ps_end_utt(ps);
  hyp = ps_get_hyp(ps, &score);
  char *ret;
  ret= strdup((char*)hyp);
  fclose(fh);
  ps_free(ps);
  cmd_ln_free_r(config);

  return (ret);
}

int main(int argc, char *argv[])
{
	char *username;
	char *path;
	asprintf(&path, "sox %s%s/%s -r 16k -c 1 --bits 16 --encoding signed-integer --endian little %s%s/new_%s", BASE_AUDIO, argv[2], argv[1], BASE_AUDIO, argv[2], argv[1]);

	system(path);
//	fprintf(stderr, " CMD  =  %s\n", path);
	asprintf(&path, "mv %s%s/new_%s %s%s/%s", BASE_AUDIO, argv[2], argv[1], BASE_AUDIO, argv[2], argv[1]);
//	fprintf(stderr, " CMD  = %s\n", path);
	system(path);
	char *hyp;
	printf("Recognized: %s\n", hyp);
	int fd = 0;
	char *filename = argv[1];
	t_cmd *cmd = NULL;

	hyp = hyp = (char*)get_hyp(argv);
	if (argc != 3 || !argv[1] || !argv[2])
			return (-1);
 	filename[strlen(argv[1]) - 4] = '\0';
	if ((cmd = get_cmd_by_hyp(hyp)) && cmd->id == 0)
	{
	  asprintf(&path, "%s%s/%s", BASE_LOG, argv[2], "log_err.txt");
		fprintf(stderr, "Failed to recognize instruction. More info in %s.\n", path);
		if ((fd = open(path, O_RDWR | O_CREAT | O_APPEND , 0666)) == -1)
			return (-1);
		dprintf(fd, "%s (%s)\n", hyp, argv[1]);
		close(fd);
		return (-1);        
	}
/*	if ((cmd = get_cmd_by_hyp(hyp)) && cmd->id == 0)
	{
		fprintf(stderr, "Failed to recognize instruction\n");
		//	return (-1);
	}
*/
//	printf("cmd->id instruction %i  \ncmd->train_sentence %s\n", cmd->id, cmd->train_sentence);
	if (write_logs_response(cmd, argv, hyp) == -1)
		return (-1);
	if (write_for_train(cmd, argv) == -1)
		return (-1);
	//execute script train_model.sh (username)
	if (update_model(argv[2]) == -1)
		return(-1);
	printf("%d", cmd->id);
	return (cmd->id);
}
//    config = cmd_ln_init(NULL, ps_args(), TRUE,
//           "-hmm", MODELDIR "/en-us/en-us",
//           "-lm", MODELDIR "/en-us/en-us.lm.bin",
//           "-jsgf", "hello_team.gram",
//           "-dict", MODELDIR "/en-us/cmudict-en-us.dict",
//           NULL);
