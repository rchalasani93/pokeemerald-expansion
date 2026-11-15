@ Roguelite ("PokeRogue-style") run mode. Logic lives in src/roguelite.c.

Roguelite_EventScript_Guide::
	lock
	faceplayer
	special Roguelite_GetRunState
	goto_if_ne VAR_RESULT, 0, Roguelite_EventScript_Resume
	buffernumberstring STR_VAR_1, VAR_ROGUELITE_BEST_WAVE
	msgbox Roguelite_Text_Intro, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Roguelite_EventScript_Decline
	msgbox Roguelite_Text_ParkParty, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Roguelite_EventScript_Decline
	special Roguelite_StartRun
	goto_if_eq VAR_RESULT, FALSE, Roguelite_EventScript_PCFull
	msgbox Roguelite_Text_DraftIntro, MSGBOX_DEFAULT
	goto Roguelite_EventScript_DraftLoop

Roguelite_EventScript_Decline:
	msgbox Roguelite_Text_Decline, MSGBOX_DEFAULT
	release
	end

Roguelite_EventScript_PCFull:
	msgbox Roguelite_Text_PCFull, MSGBOX_DEFAULT
	release
	end

Roguelite_EventScript_Resume:
	goto_if_eq VAR_ROGUELITE_WAVE, 0, Roguelite_EventScript_ResumeDraft
	buffernumberstring STR_VAR_1, VAR_ROGUELITE_WAVE
	msgbox Roguelite_Text_Resume, MSGBOX_DEFAULT
	goto Roguelite_EventScript_BetweenWaves

Roguelite_EventScript_ResumeDraft:
	msgbox Roguelite_Text_ResumeDraft, MSGBOX_DEFAULT
	goto Roguelite_EventScript_DraftLoop

@ ---------------------------------------------------------------------------
@ Starter draft: spend ROGUELITE_STARTER_BUDGET points on up to six Pokémon.

Roguelite_EventScript_DraftLoop:
	special Roguelite_BuildStarterMenu
	message Roguelite_Text_DraftPrompt
	waitmessage
	dynmultistack 0, 0, TRUE, 7, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	special Roguelite_PickStarter
	switch VAR_RESULT
	case ROGUELITE_PICK_CONTINUE, Roguelite_EventScript_DraftPicked
	case ROGUELITE_PICK_DONE, Roguelite_EventScript_DraftDone
	case ROGUELITE_PICK_TOO_EXPENSIVE, Roguelite_EventScript_DraftTooExpensive
	case ROGUELITE_PICK_NEED_ONE, Roguelite_EventScript_DraftNeedOne
	case ROGUELITE_PICK_ALREADY, Roguelite_EventScript_DraftAlready
	goto Roguelite_EventScript_DraftLoop

Roguelite_EventScript_DraftAlready:
	msgbox Roguelite_Text_DraftAlready, MSGBOX_DEFAULT
	goto Roguelite_EventScript_DraftLoop

Roguelite_EventScript_DraftPicked:
	playfanfare MUS_LEVEL_UP
	message Roguelite_Text_Joined
	waitmessage
	waitfanfare
	goto Roguelite_EventScript_DraftLoop

Roguelite_EventScript_DraftTooExpensive:
	msgbox Roguelite_Text_TooExpensive, MSGBOX_DEFAULT
	goto Roguelite_EventScript_DraftLoop

Roguelite_EventScript_DraftNeedOne:
	msgbox Roguelite_Text_NeedOne, MSGBOX_DEFAULT
	goto Roguelite_EventScript_DraftLoop

Roguelite_EventScript_DraftDone:
	msgbox Roguelite_Text_RunBegins, MSGBOX_DEFAULT
	setvar VAR_ROGUELITE_ROUTE, ROGUELITE_ROUTE_WILD
	goto Roguelite_EventScript_NextWave

@ ---------------------------------------------------------------------------
@ Wave loop

Roguelite_EventScript_NextWave:
	special Roguelite_PrepareWave
	copyvar VAR_0x8005, VAR_RESULT
	goto_if_eq VAR_0x8004, FALSE, Roguelite_EventScript_AnnounceWave
	msgbox Roguelite_Text_NewBiome, MSGBOX_DEFAULT
Roguelite_EventScript_AnnounceWave:
	switch VAR_0x8005
	case ROGUELITE_WAVE_KIND_FINAL, Roguelite_EventScript_AnnounceFinal
	case ROGUELITE_WAVE_KIND_BOSS, Roguelite_EventScript_AnnounceBoss
	case ROGUELITE_WAVE_KIND_TRAINER, Roguelite_EventScript_TrainerWave
	case ROGUELITE_WAVE_KIND_REST, Roguelite_EventScript_RestWave
	msgbox Roguelite_Text_Wave, MSGBOX_DEFAULT
	goto Roguelite_EventScript_Fight

Roguelite_EventScript_TrainerWave:
	msgbox Roguelite_Text_TrainerWave, MSGBOX_DEFAULT
	closemessage
	cleartrainerflag TRAINER_ROGUELITE
	trainerbattle_no_intro TRAINER_ROGUELITE, Roguelite_Text_TrainerDefeat
	goto Roguelite_EventScript_AfterFight

Roguelite_EventScript_RestWave:
	playfanfare MUS_HEAL
	msgbox Roguelite_Text_RestWave, MSGBOX_DEFAULT
	waitfanfare
	special Roguelite_AfterWave
	goto Roguelite_EventScript_BetweenWaves

Roguelite_EventScript_AnnounceBoss:
	msgbox Roguelite_Text_BossWave, MSGBOX_DEFAULT
	goto Roguelite_EventScript_Fight

Roguelite_EventScript_AnnounceFinal:
	msgbox Roguelite_Text_FinalWave, MSGBOX_DEFAULT
Roguelite_EventScript_Fight:
	closemessage
	special Roguelite_StartWaveBattle
Roguelite_EventScript_AfterFight:
	special Roguelite_AfterWave
	switch VAR_RESULT
	case ROGUELITE_WAVE_LOST, Roguelite_EventScript_RunLost
	case ROGUELITE_WAVE_RUN_WON, Roguelite_EventScript_RunWon
	case ROGUELITE_WAVE_BOSS_FLED, Roguelite_EventScript_BossFled
	goto_if_eq VAR_0x8004, FALSE, Roguelite_EventScript_Reward
	msgbox Roguelite_Text_BossCleared, MSGBOX_DEFAULT
Roguelite_EventScript_Reward:
	special Roguelite_BuildRewardMenu
	message Roguelite_Text_ChooseReward
	waitmessage
	dynmultistack 0, 0, TRUE, 3, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	special Roguelite_GiveReward
	goto_if_eq VAR_RESULT, FALSE, Roguelite_EventScript_BagFull
	playse SE_SELECT
	msgbox Roguelite_Text_GotReward, MSGBOX_DEFAULT
	goto Roguelite_EventScript_BetweenWaves

Roguelite_EventScript_BagFull:
	msgbox Roguelite_Text_BagFull, MSGBOX_DEFAULT
Roguelite_EventScript_BetweenWaves:
	message Roguelite_Text_WhatNext
	waitmessage
	dynmultichoice 0, 0, TRUE, 3, 0, DYN_MULTICHOICE_CB_NONE, Roguelite_Text_OptNextWave, Roguelite_Text_OptBreak, Roguelite_Text_OptGiveUp
	switch VAR_RESULT
	case 1, Roguelite_EventScript_TakeBreak
	case 2, Roguelite_EventScript_ConfirmGiveUp
Roguelite_EventScript_ChooseRoute:
	special Roguelite_BuildRouteMenu
	message Roguelite_Text_ChooseRoute
	waitmessage
	dynmultistack 0, 0, TRUE, 3, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	special Roguelite_SetRoute
	goto Roguelite_EventScript_NextWave

Roguelite_EventScript_TakeBreak:
	msgbox Roguelite_Text_Break, MSGBOX_DEFAULT
	release
	end

Roguelite_EventScript_ConfirmGiveUp:
	msgbox Roguelite_Text_ConfirmGiveUp, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Roguelite_EventScript_BetweenWaves
	setvar VAR_0x8004, ROGUELITE_END_GAVE_UP
	goto Roguelite_EventScript_EndRun

Roguelite_EventScript_BossFled:
	msgbox Roguelite_Text_BossFled, MSGBOX_DEFAULT
	goto Roguelite_EventScript_NextWave

Roguelite_EventScript_RunLost:
	msgbox Roguelite_Text_RunLost, MSGBOX_DEFAULT
	setvar VAR_0x8004, ROGUELITE_END_LOST
	goto Roguelite_EventScript_EndRun

Roguelite_EventScript_RunWon:
	playfanfare MUS_OBTAIN_SYMBOL
	msgbox Roguelite_Text_RunWon, MSGBOX_DEFAULT
	waitfanfare
	setvar VAR_0x8004, ROGUELITE_END_WON
Roguelite_EventScript_EndRun:
	special Roguelite_EndRun
	goto_if_eq VAR_RESULT, FALSE, Roguelite_EventScript_EndRunNoRecord
	msgbox Roguelite_Text_NewRecord, MSGBOX_DEFAULT
	goto Roguelite_EventScript_EndRunDone
Roguelite_EventScript_EndRunNoRecord:
	msgbox Roguelite_Text_RunOver, MSGBOX_DEFAULT
Roguelite_EventScript_EndRunDone:
	msgbox Roguelite_Text_CoinsEarned, MSGBOX_DEFAULT
	msgbox Roguelite_Text_PartyBack, MSGBOX_DEFAULT
	release
	end

@ ---------------------------------------------------------------------------
@ Text

Roguelite_Text_Intro:
	.string "I study Pokémon that live in endless\n"
	.string "loops of battle… Want to try a run?\p"
	.string "Draft a team, clear wave after wave,\n"
	.string "and pick one reward after each.\p"
	.string "Any Pokémon you catch can be drafted\n"
	.string "in later runs.\p"
	.string "Every 10 waves a boss guards a new\n"
	.string "biome. Reach wave 50 to win.\p"
	.string "Your best so far: wave {STR_VAR_1}.\n"
	.string "Ready to begin?$"

Roguelite_Text_ParkParty:
	.string "Your current party will wait in the\n"
	.string "PC until the run ends. Okay?$"

Roguelite_Text_Decline:
	.string "Come back when you feel like\n"
	.string "testing your luck.$"

Roguelite_Text_PCFull:
	.string "Your PC is too full to hold your\n"
	.string "party. Free up some space first.$"

Roguelite_Text_Resume:
	.string "Welcome back! You're on wave\n"
	.string "{STR_VAR_1}. Let's keep going.$"

Roguelite_Text_ResumeDraft:
	.string "You were still drafting your team.\n"
	.string "Let's finish that.$"

Roguelite_Text_DraftIntro:
	.string "Spend your points on a team.\n"
	.string "Stronger lines cost more points.\p"
	.string "All 27 classic starters are here,\n"
	.string "plus every Pokémon family you've\l"
	.string "caught before.$"

Roguelite_Text_DraftPrompt:
	.string "Points left: {STR_VAR_1}. Page {STR_VAR_2}/{STR_VAR_3}.\n"
	.string "Who joins your team?$"

Roguelite_Text_DraftAlready:
	.string "{STR_VAR_2} is already on your team.$"

Roguelite_Text_Joined:
	.string "{STR_VAR_2} joined your team!$"

Roguelite_Text_TooExpensive:
	.string "You can't afford {STR_VAR_2}.$"

Roguelite_Text_NeedOne:
	.string "Pick at least one Pokémon first!$"

Roguelite_Text_RunBegins:
	.string "Your team is set. The run begins!$"

Roguelite_Text_NewBiome:
	.string "You've entered a new biome:\n"
	.string "{STR_VAR_2}!$"

Roguelite_Text_Wave:
	.string "Wave {STR_VAR_1} - {STR_VAR_2}$"

Roguelite_Text_BossWave:
	.string "Wave {STR_VAR_1} - BOSS!\n"
	.string "A powerful Pokémon blocks the way!$"

Roguelite_Text_FinalWave:
	.string "Wave {STR_VAR_1} - FINAL BOSS!\n"
	.string "The sky itself begins to roar…$"

Roguelite_Text_BossCleared:
	.string "The boss is down! Your team\n"
	.string "catches its breath and recovers.$"

Roguelite_Text_BossFled:
	.string "There's no running from a boss!\n"
	.string "Face it again!$"

Roguelite_Text_ChooseReward:
	.string "Wave cleared! Choose a reward.$"

Roguelite_Text_GotReward:
	.string "You got {STR_VAR_1}!$"

Roguelite_Text_BagFull:
	.string "Your Bag is full, so the reward\n"
	.string "was lost…$"

Roguelite_Text_WhatNext:
	.string "What will you do?$"

Roguelite_Text_OptNextWave:
	.string "Next wave$"

Roguelite_Text_ChooseRoute:
	.string "Which way will you go?$"

Roguelite_Text_TrainerWave:
	.string "Wave {STR_VAR_1} - A ROGUE trainer\n"
	.string "challenges you!$"

Roguelite_Text_TrainerDefeat:
	.string "Tch… you're tougher than\n"
	.string "you look.$"

Roguelite_Text_RestWave:
	.string "Wave {STR_VAR_1} - Rest stop.\n"
	.string "Your team is fully healed!$"

Roguelite_Text_OptBreak:
	.string "Take a break$"

Roguelite_Text_OptGiveUp:
	.string "End the run$"

Roguelite_Text_Break:
	.string "Take your time. Sort your Bag, use\n"
	.string "the hub, save… Talk to me to continue.$"

Roguelite_Text_ConfirmGiveUp:
	.string "End the run here? Your run team\n"
	.string "will be released.$"

Roguelite_Text_RunLost:
	.string "Your whole team fainted…\n"
	.string "The run is over.$"

Roguelite_Text_RunWon:
	.string "Incredible! You cleared all 50\n"
	.string "waves! The run is yours!$"

Roguelite_Text_NewRecord:
	.string "You reached wave {STR_VAR_1}.\n"
	.string "That's a new personal best!$"

Roguelite_Text_RunOver:
	.string "You reached wave {STR_VAR_1}.$"

Roguelite_Text_CoinsEarned:
	.string "You earned {STR_VAR_2} Rogue Coins.\n"
	.string "Spend them with the Builder.$"

Roguelite_Text_PartyBack:
	.string "Your run team has gone free, and\n"
	.string "your own party is back with you.$"
