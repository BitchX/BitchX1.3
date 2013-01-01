#include "acro.h"

srec *scores = NULL, *gscores = NULL;
prec *player = NULL;
vrec *voter = NULL;
grec *game = NULL;

int Acro_Init(IrcCommandDll **intp, Function_ptr *global_table)
{
	initialize_module("Acromania");
	add_module_proc(RAW_PROC, "acro", "PRIVMSG", NULL, 0, 0, acro_main, NULL);
	add_module_proc(COMMAND_PROC, "scores", "scores", NULL, 0, 0, put_scores, NULL);

	gscores = read_scores();
	if (!game)
		game = init_acro(game);
	put_it("BitchX Acromania dll v0.9b by By-Tor loaded...");
	return 0;
}

static int acro_main (char *comm, char *from, char *userhost, char **args)
{
	if (*args[1] && !strncasecmp(args[1], "acro ", 5) && !strcasecmp(args[0], get_server_nickname(from_server)))
	{
		PasteArgs(args, 1);
		switch(game->progress) {
			case 0:
				send_to_server("notice %s :Sorry, no game in progress.", from);
				break;
			case 1:
				if (valid_acro(game, args[1]+5))
					player = take_acro(game, player, from, userhost, args[1]+5);
				else
					send_to_server("PRIVMSG %s :Invalid acronym", from);
				break;
			case 2:
				voter = take_vote(game, voter, player, from, userhost, args[1]+5);
				break;
		}
		return 1;
	}
	if (*args[0] == '#' && *args[1] && !my_stricmp(args[1], "start"))
	{
		if (game && game->progress)
		{
			send_to_server("PRIVMSG %s :Game already in progress, %s. Puzzle is: %s", from, from, game->nym);
			return 0;
		}
		make_acro(game);
		game->progress = 1;
		send_to_server("PRIVMSG %s :Round %d", args[0], game->round);
		send_to_server("PRIVMSG %s :The acronym for this round is %s. You have 60 seconds.", args[0], game->nym);
		send_to_server("PRIVMSG %s :/msg %s \"acro <your answer>\"", args[0], get_server_nickname(from_server));
		add_timer(0, "Acro", 60 * 1000, 1, (int(*)(void *))warn_acro, m_sprintf("%s", args[0]), NULL, NULL, "acro");
	}
	return 0;
}


BUILT_IN_DLL(put_scores)
{
	srec *stmp = NULL;
	if (scores) {
		put_it("Start Normal scores");
		for (stmp = scores; stmp; stmp = stmp->next)
			put_it("Nick: %s\tScore: %lu", stmp->nick, stmp->score);
		put_it("End scores");
	}
/*
	if (gscores) {
		put_it("GLOBAL SCORES START HERE!"); 
		for (stmp = gscores; stmp; stmp = stmp->next)
			put_it("Nick: %s\tScore %lu", stmp->nick, stmp->score);
		put_it("GLOBAL SCORES END HERE!"); 
	}
*/
}

void warn_acro(char *chan)
{
	send_to_server("PRIVMSG %s :30 seconds! Puzzle is: %s", chan, game->nym);
	add_timer(0, "Acro", 30 * 1000, 1, (int(*)(void *))start_vote, m_sprintf("%s", chan), NULL, NULL, "acro");
}

void start_vote(char *chan)
{
	if (game->players >= MINPLAYERS)
	{
		send_to_server("PRIVMSG %s :Time's up, lets vote!\r\nPRIVMSG %s :/msg %s \"acro #\" to vote", chan, chan, get_server_nickname(from_server));
		game->progress = 2;
		show_acros(player, chan);
		add_timer(0, "Acro", 30 * 1000, 1, (int(*)(void *))warn_vote, m_sprintf("%s", chan), NULL, NULL, "acro");
	}
	else if (game->extended < EXTENSIONS)
	{
		send_to_server("PRIVMSG %s :Aww, too few players! Puzzle is: %s", chan, game->nym);
		add_timer(0, "Acro", 30 * 1000, 1, (int(*)(void *))start_vote, m_sprintf("%s", chan), NULL, NULL, "acro");
		game->extended++;
	}
	else
	{
		send_to_server("PRIVMSG %s :Not enough players, ending game...", chan);
		free_round(&player, &voter);
		game->players = 0;
		game->progress = 0;
	}
}
 
void warn_vote(char *chan)
{
	send_to_server("PRIVMSG %s :30 seconds left to vote!", chan);
	add_timer(0, "Acro", 30 * 1000, 1, (int(*)(void *))end_voting, m_sprintf("%s", chan), NULL, NULL, "acro");
}

void end_voting(char *chan)
{
	put_it("END_VOTING");
	send_to_server("PRIVMSG %s :Voting complete, sorting scores...", chan);
	gscores = end_vote(voter, player, gscores);
	scores = end_vote(voter, player, scores);
	write_scores(gscores);
	show_scores(game, scores, gscores, chan);
	free_round(&player, &voter);
	if (player)
	{
		put_it("Player was non-null!!");
		player = NULL;
	}
	if (voter)
	{
		put_it("voter was non-null!!");
		voter = NULL;
	}
	if (game->round < game->rounds)
	{
		init_acro(game);
		send_to_server("PRIVMSG %s :Round %d", chan, game->round);
		send_to_server("PRIVMSG %s :The acronym for this round is %s. You have 60 seconds.", chan, game->nym);
		send_to_server("PRIVMSG %s :/msg %s \"acro <your answer>\"", chan, get_server_nickname(from_server));
		add_timer(0, "Acro", 60 * 1000, 1, (int(*)(void *))warn_acro, m_sprintf("%s", chan), NULL, NULL, "acro");
	}		
	else
	{
		game->round = 1;
		game->progress = 0;
		free_score(&scores);
		new_free(&game->nym);
		init_acro(game);
	}
}

grec *init_acro(grec *gtmp)
{
	if (!gtmp)
		gtmp = (grec *)new_malloc(sizeof(grec));
	if (gtmp->nym)
	{
		gtmp->progress = 1;
		gtmp->round++;
		gtmp->players = 0;
		gtmp->extended = 0;
		new_free(&gtmp->nym);
		make_acro(gtmp);
	}
	else
	{	
		gtmp->progress = 0;
		gtmp->round = 1;
		gtmp->rounds = ROUNDS;
		gtmp->players = 0;
		gtmp->extended = 0;
		gtmp->top = TOP;
		gtmp->maxlen = MAXLEN;
		gtmp->nym = NULL;
	}
	return gtmp;
}

void make_acro(grec *acro)
{
	int i, r;
	char *p;
	if (acro->nym)
		new_free(&acro->nym);
	r = MINLENGTH+(int)((float) (1+MAXLENGTH-MINLENGTH)*random()/(RAND_MAX+1.0));
	p = acro->nym = (char *)new_malloc(r+1);
	for (i = 0; i < r; i++)
		*p++ = letters[(int)((float)(strlen(letters))*random()/(RAND_MAX+1.0))];
	return;
}

int valid_acro(grec *acro, char *p)
{
	int spaces = 0, len = 0, gotsp = 1;
	if (!p)
		return 0;
	if (!acro)
		return 0;
	for (; *p; p++)
	{
		if (isalpha(*p))
		{
			len++;
			if (gotsp && (toupper(*p) != acro->nym[spaces]))
				return 0;
			gotsp = 0;
		}
		else if (*p == 32)
		{
			if (!gotsp)
			{
				spaces++;
				gotsp = 1;
			}
		}
		else return 0;
	}
	if ((len > strlen(acro->nym)) && (spaces+1 == strlen(acro->nym)))
		return 1;
	return 0;
}

prec *take_acro(grec *acro, prec *players, char *nick, char *host, char *stuff)
{
	prec *tmp, *last = NULL;
	if (!players)
	{
		players = (prec *)new_malloc(sizeof(prec));
		players->nick = (char *)new_malloc(strlen(nick)+1);
		players->host = (char *)new_malloc(strlen(host)+1);
		players->acro = (char *)new_malloc(strlen(stuff)+1);
		strcpy(players->nick, nick);
		strcpy(players->host, host);
		strcpy(players->acro, stuff);
		send_to_server("PRIVMSG %s :Answer set to \"%s\"\r\nPRIVMSG %s :You are player #%d", nick, stuff, nick, ++acro->players);
		return players;
	}
	for (tmp = players; tmp; tmp = tmp->next)
	{
		if (tmp->host && !strcasecmp(host, tmp->host))
		{
			if (tmp->acro && !strcasecmp(stuff, tmp->acro))
			{
				send_to_server("PRIVMSG %s :Your answer is alreay \"%s\"", nick, stuff);
				return players;
			}
			else if (tmp->last && !strcasecmp(stuff, tmp->last))
			{
				RESIZE(tmp->acro, char, strlen(stuff)+1);
				strcpy(tmp->acro, stuff);
				send_to_server("PRIVMSG %s :Answer changed to \"%s\"", nick, stuff);
				new_free(&tmp->last);
				return players;
			}
			else {
				tmp->last = (char *)new_malloc(strlen(stuff)+1);
				strcpy(tmp->last, stuff);
				send_to_server("PRIVMSG %s :You already submitted an answer, submit once more to change.", nick);
				return players;
			}
		}
		last = tmp;
	}
	if (acro->players < MAXPLAYERS && last)
	{
		tmp = last->next = (prec *)new_malloc(sizeof(prec));
		tmp->nick = (char *)new_malloc(strlen(nick)+1);
		tmp->host = (char *)new_malloc(strlen(host)+1);
		tmp->acro = (char *)new_malloc(strlen(stuff)+1);
		strcpy(tmp->nick, nick);
		strcpy(tmp->host, host);
		strcpy(tmp->acro, stuff);
		send_to_server("PRIVMSG %s :Answer set to \"%s\"\r\nPRIVMSG %s :You are player #%d", nick, stuff, nick, ++acro->players);
	}
	else
		send_to_server("PRIVMSG %s :Sorry, too many players.", nick);
	return players;
}

vrec *take_vote(grec *acro, vrec *voters, prec *players, char *nick, char *host, char *num)
{
	vrec *tmp = NULL, *last = voters;
	int i;
  if (atoi(num) > acro->players || atoi(num) < 1)
	{
		send_to_server("PRIVMSG %s :No such answer...", nick);
		return voters;
	}
	for (i = 1; i < atoi(num); i++)
		players = players->next;
	if (players->nick && nick && !strcasecmp(players->nick, nick))
	{
		send_to_server("PRIVMSG %s :You can't vote for yourself.", nick);
		return voters;
	}
	if (!voters)
	{
		voters = (vrec *)new_malloc(sizeof(vrec));
		voters->nick = (char *)new_malloc(strlen(nick)+1);
		voters->host = (char *)new_malloc(strlen(host)+1);
		voters->vote = atoi(num)-1;
		strcpy(voters->nick, nick);
		strcpy(voters->host, host);
		send_to_server("PRIVMSG %s :Vote recorded...", nick);
		return voters;
	}
	for (tmp = voters; tmp; tmp = tmp->next)
	{
		if ((tmp->nick && !strcasecmp(tmp->nick, nick)) || (tmp->host && !strcasecmp(tmp->host, host)))
		{
			send_to_server("PRIVMSG %s :You already voted.", nick);
			return voters;
		}
		last = tmp;
	}
	if (last && !last->next)
	{
		last = last->next = (vrec *)new_malloc(sizeof(vrec));
		last->nick = (char *)new_malloc(strlen(nick)+1+sizeof(char *));
		last->host = (char *)new_malloc(strlen(host)+1+sizeof(char *));
		last->vote = atoi(num)-1;
		strcpy(last->nick, nick);
		strcpy(last->host, host);
		send_to_server("PRIVMSG %s :Vote recorded...", nick);
	}
	return voters;
}

srec *read_scores()
{
	srec *tmp, *tmp2;
	char buff[100], *p;
	FILE *sf;
	tmp = tmp2 = (srec *)new_malloc(sizeof(srec));
	memset(buff, 0, sizeof(buff));
	sf = fopen(SCOREFILE, "r");
	if (!sf)
		return 0;
	while (!feof(sf))
	{
		if (fgets(buff, 51, sf))
		{
			if (tmp->nick)
				tmp = tmp->next = (srec *)new_malloc(sizeof(srec));
			if (buff[strlen(buff)-1] == '\n')
				buff[strlen(buff)-1] = 0;
			if (!*buff)
				break;
			if ((p = (char *)strchr(buff, ',')))
				*p++ = 0;
			else if (!p)
			{
				return tmp2;
				fclose(sf);
			}
			tmp->nick = (char *)new_malloc(strlen(buff+1));
			strcpy(tmp->nick, buff);
			if (p)
				tmp->score = strtoul(p, NULL, 10);
		}
		else
			break;
	}
	fclose(sf);
	return tmp2;
}

int write_scores(srec *tmp)
{
	FILE *sf;
	if (!tmp)
		return 0;
	tmp = sort_scores(tmp);
	sf = fopen(SCOREFILE, "w");
	if (!sf)
		return 0;
	for (; tmp; tmp = tmp->next)
		if (tmp->score > 0)
			fprintf(sf, "%s,%lu\n", tmp->nick, tmp->score);
	fclose(sf);
	return 1;
}

/*
 * Here we take the votes from the current round and translate them into
 * scores for the players. gscores is the score per game, while scores
 * is the overall score for all games... So we call it with a ptr to the
 * score record we want to update and it makes it more useful :)
 * The dual for () loop is probably inefficient, but its all I can think of
 * right now ;), besides we aren't talking about 10,000 members here

 * Eventually I'll convert this to support variable arg lengths and thus
 * update for multiple scores with one call...
 * This function is a mess right now, I'll clean it up later.
*/

srec *end_vote(vrec *voters, prec *players, srec *stmp)
{
	int i, gotscore;
	vrec *tmp = NULL;
	prec *tmp2 = NULL;
	srec *tmp3 = NULL, *last;
	if (!stmp && voters && players)
		stmp = (srec *)new_malloc(sizeof(srec));
	for (tmp = voters; tmp; tmp = tmp->next)
	{
		gotscore = 0;
		tmp2 = players;
		for (i = 0; i < tmp->vote; i++)
			tmp2 = tmp2->next;
		if (stmp && !stmp->nick)
		{
			stmp->nick = (char *)new_malloc(strlen(tmp2->nick)+1);
			strcpy(stmp->nick, tmp2->nick);
			stmp->score = 1;
			continue;
		}
		for (tmp3 = last = stmp; tmp3; tmp3 = tmp3->next)
		{
			if (tmp2->nick && tmp3->nick && !strcasecmp(tmp2->nick, tmp3->nick))
			{
				tmp3->score++;
				gotscore = 1;
				break;
			}
			last = tmp3;
		}
		if (!gotscore)
		{
			tmp3 = last->next = (srec *)new_malloc(sizeof(srec));
			tmp3->nick = (char *)new_malloc(strlen(tmp2->nick)+1);
			strcpy(tmp3->nick, tmp2->nick);
			tmp3->score = 1;
		}
	}
	return stmp;
}

srec *sort_scores(srec *stmp)
{
	int i = 0;
	srec *tmp;
	srec **sort, **ctmp;
	if (!stmp->next)
		return stmp;
	for (tmp = stmp; tmp; tmp = tmp->next)
		i++;
	ctmp = sort = (srec **)new_malloc(i*sizeof(srec *));
	put_it("START SORTING");
	put_scores(NULL, NULL, NULL, NULL, NULL);
	for (tmp = stmp; tmp; tmp = tmp->next)
		*ctmp++ = tmp;
        qsort((void *)sort, i+1, sizeof(srec *), (int (*)(const void *, const void *))comp_score);
	ctmp = sort;
	for (tmp = *ctmp++; *ctmp; ctmp++)
		tmp = tmp->next = *ctmp;
	tmp->next = NULL;
	tmp = *sort;
	new_free(&sort);
	put_scores(NULL, NULL, NULL, NULL, NULL);
	put_it("END SCORES");
 	return tmp;
}

/* 
 * Here we sort deeze babys ... The return values are "opposite" so we can
 * sort in descending order instead of ascending... Stupid declarations had 
 * me going for a while, no wonder it didnt sort right at first! :)
*/

int comp_score(srec **one, srec **two)
{
  if ((*one)->score > (*two)->score)
    return -1;
  if ((*one)->score < (*two)->score)
    return 1;
  else
    return strcasecmp((*one)->nick, (*two)->nick);
}

void show_acros(prec *players, char *chan)
{
	prec *tmp;
	int i = 1;
	char *line, buff[201];
	if (!players)
		return;
	line = (char *)new_malloc(513);
	memset(buff, 0, sizeof(buff));
	for (tmp = players; tmp; tmp = tmp->next)
	{
		snprintf(buff, 198, "PRIVMSG %s :%2d: %s", chan, i++, tmp->acro);
		strcat(buff, "\r\n");
		if (strlen(line)+strlen(buff) >= 512)
		{
			send_to_server("%s", line);
			memset(line, 0, 513);
		}
		strcat(line, buff);
		memset(buff, 0, sizeof(buff));
	}
	if (line)
		send_to_server("%s", line);
	new_free(&line);
} 

void show_scores(grec *acro, srec *score, srec *gscore, char *chan)
{
	char *line, buff[201];
	int i;
	line = (char *)new_malloc(513);
	memset(buff, 0, sizeof(buff));
	if (score)
		score = sort_scores(score);
	if (gscore && (acro->round >= acro->rounds))
		gscore = sort_scores(gscore);
	if (acro->round < acro->rounds)
		sprintf(line, "PRIVMSG %s :Scores for round %d\r\nPRIVMSG %s :Nick        Score\r\nPRIVMSG %s :-----------------\r\n", chan, acro->round, chan, chan);
	else
		sprintf(line, "PRIVMSG %s :Game over, tallying final scores...\r\nPRIVMSG %s :   Game Score          Overall Score\r\nPRIVMSG %s :Nick        Score    Nick        Score\r\nPRIVMSG %s :-----------------    -----------------\r\n", chan, chan, chan, chan);
	for (i = 0; i < acro->top && (score || gscore); i++)
	{
		if ((acro->round < acro->rounds) && score)
		{
			snprintf(buff, 198, "PRIVMSG %s :%-9s    %lu", chan, score->nick, score->score);
			strcat(buff, "\r\n");
			score = score->next;
		}
		else if (acro->round == acro->rounds && (score || gscore)) 
		{
			if (!score && gscore)
			{
				snprintf(buff, 198, "PRIVMSG %s :                     %-9s   %lu", chan, gscore->nick, gscore->score);
				strcat(buff, "\r\n");
				gscore = gscore->next;
			}
			else if (score && !gscore)
			{
				snprintf(buff, 198, "PRIVMSG %s :%-9s    %lu", chan, score->nick, score->score);
				strcat(buff, "\r\n");
				score = score->next;
			}
			else if (gscore && score)
			{
				snprintf(buff, 198, "PRIVMSG %s :%-9s    %-5lu   %-9s    %lu", chan, score->nick, score->score, gscore->nick, gscore->score);
				strcat(buff, "\r\n");
				gscore = gscore->next;
				score = score->next;
			}
		}
		if (strlen(line)+strlen(buff) >= 512)
		{
			send_to_server("%s", line);
			memset(line, 0, 513);
		}
		strcat(line, buff);
		memset(buff, 0, sizeof(buff));
	}
	if (line)
		send_to_server("%s", line);
	new_free(&line);
} 

void free_round(prec **players, vrec **voters)
{
	prec *ptmp, *ptmp2;
	vrec *vtmp, *vtmp2;
	if (players && *players) {
		for (ptmp = *players; ptmp;)
		{
			if (ptmp->nick)
				new_free(&ptmp->nick);
			if (ptmp->host)
  			new_free(&ptmp->host);
  		if (ptmp->acro)
  			new_free(&ptmp->acro);
  		if (ptmp->last)
  			new_free(&ptmp->last);
  		if (ptmp->next)
  			ptmp2 = ptmp->next;
  		else
  			ptmp2 = NULL;
  		new_free(&ptmp);
  		ptmp = ptmp2;
  	}
  	*players = NULL;
  }
  if (voters && *voters) {
  	for (vtmp = *voters; vtmp;)
  	{
  		if (vtmp->nick)
  			new_free(&vtmp->nick);
  		if (vtmp->host)
  			new_free(&vtmp->host);
  		if (vtmp->next)
  			vtmp2 = vtmp->next;
  		else
  			vtmp2 = NULL;
  		new_free(&vtmp);
  		vtmp = vtmp2;
  	}
  	*voters = NULL;
  }
}

void free_score(srec **score)
{
	srec *stmp, *stmp2;
	for (stmp = *score; stmp;)
	{
		if (stmp->nick)
			new_free(&stmp->nick);
		if (stmp->next)
			stmp2 = stmp->next;
		else
			stmp2 = NULL;
		new_free(&stmp);
		stmp = stmp2;
	}
	*score = NULL;
}
