@ Roguelite hub (Emerald Rogue-style). A closed town with no exits where new
@ games start. Areas are built with Rogue Coins earned in runs; each one shows
@ up as an NPC once built and as a construction site before that.

RogueHub_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, RogueHub_OnTransition
	.byte 0

RogueHub_OnTransition:
	call_if_unset FLAG_SYS_POKEMON_GET, RogueHub_EventScript_FirstVisit
	call RogueHub_EventScript_SetAreaVisibility
	end

@ New games skip the story intro, so turn on what the menus need.
RogueHub_EventScript_FirstVisit:
	setflag FLAG_SYS_POKEMON_GET
	setflag FLAG_SYS_POKEDEX_GET
	setflag FLAG_SYS_B_DASH
	setflag FLAG_RECEIVED_RUNNING_SHOES
	special EnableNationalPokedex
	return

@ FLAG_TEMP_* hide objects: odd = the area's NPC, even = its construction site.
RogueHub_EventScript_SetAreaVisibility:
	call_if_unset FLAG_ROGUE_HUB_MART, RogueHub_EventScript_HideMart
	call_if_set FLAG_ROGUE_HUB_MART, RogueHub_EventScript_HideMartSite
	call_if_unset FLAG_ROGUE_HUB_HEALER, RogueHub_EventScript_HideHealer
	call_if_set FLAG_ROGUE_HUB_HEALER, RogueHub_EventScript_HideHealerSite
	call_if_unset FLAG_ROGUE_HUB_TUTOR, RogueHub_EventScript_HideTutor
	call_if_set FLAG_ROGUE_HUB_TUTOR, RogueHub_EventScript_HideTutorSite
	call_if_unset FLAG_ROGUE_HUB_ABILITY_LAB, RogueHub_EventScript_HideAbilityLab
	call_if_set FLAG_ROGUE_HUB_ABILITY_LAB, RogueHub_EventScript_HideAbilityLabSite
	call_if_eq VAR_ROGUE_HUB_SUPPLY_LEVEL, 0, RogueHub_EventScript_HideSupply
	call_if_ne VAR_ROGUE_HUB_SUPPLY_LEVEL, 0, RogueHub_EventScript_HideSupplySite
	call_if_eq VAR_ROGUE_HUB_DRAFT_LEVEL, 0, RogueHub_EventScript_HideDraft
	call_if_ne VAR_ROGUE_HUB_DRAFT_LEVEL, 0, RogueHub_EventScript_HideDraftSite
	return

RogueHub_EventScript_HideMart:
	setflag FLAG_TEMP_11
	return
RogueHub_EventScript_HideMartSite:
	setflag FLAG_TEMP_12
	return
RogueHub_EventScript_HideHealer:
	setflag FLAG_TEMP_13
	return
RogueHub_EventScript_HideHealerSite:
	setflag FLAG_TEMP_14
	return
RogueHub_EventScript_HideTutor:
	setflag FLAG_TEMP_15
	return
RogueHub_EventScript_HideTutorSite:
	setflag FLAG_TEMP_16
	return
RogueHub_EventScript_HideAbilityLab:
	setflag FLAG_TEMP_17
	return
RogueHub_EventScript_HideAbilityLabSite:
	setflag FLAG_TEMP_18
	return
RogueHub_EventScript_HideSupply:
	setflag FLAG_TEMP_19
	return
RogueHub_EventScript_HideSupplySite:
	setflag FLAG_TEMP_1A
	return
RogueHub_EventScript_HideDraft:
	setflag FLAG_TEMP_1B
	return
RogueHub_EventScript_HideDraftSite:
	setflag FLAG_TEMP_1C
	return

@ ---------------------------------------------------------------------------
@ Builder

RogueHub_EventScript_Builder::
	lock
	faceplayer
	msgbox RogueHub_Text_BuilderIntro, MSGBOX_DEFAULT
RogueHub_EventScript_BuilderMenu:
	special Roguelite_BuildHubAreaMenu
	message RogueHub_Text_BuilderPrompt
	waitmessage
	dynmultistack 0, 0, TRUE, 7, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	special Roguelite_BuyHubArea
	switch VAR_RESULT
	case ROGUELITE_BUILD_DONE, RogueHub_EventScript_Built
	case ROGUELITE_BUILD_TOO_POOR, RogueHub_EventScript_TooPoor
	case ROGUELITE_BUILD_MAXED, RogueHub_EventScript_Maxed
	msgbox RogueHub_Text_BuilderBye, MSGBOX_DEFAULT
	release
	end

RogueHub_EventScript_Built:
	playfanfare MUS_OBTAIN_ITEM
	msgbox RogueHub_Text_Built, MSGBOX_DEFAULT
	waitfanfare
	closemessage
	@ Reload the hub so the new area shows up where its site was
	fadescreen FADE_TO_BLACK
	playse SE_BANG
	getplayerxy VAR_0x8005, VAR_0x8006
	warpsilent MAP_ROGUE_HUB, VAR_0x8005, VAR_0x8006
	end

RogueHub_EventScript_TooPoor:
	msgbox RogueHub_Text_TooPoor, MSGBOX_DEFAULT
	goto RogueHub_EventScript_BuilderMenu

RogueHub_EventScript_Maxed:
	msgbox RogueHub_Text_Maxed, MSGBOX_DEFAULT
	goto RogueHub_EventScript_BuilderMenu

@ ---------------------------------------------------------------------------
@ Construction sites (areas not built yet)

RogueHub_EventScript_ConstructionMart::
	bufferstring STR_VAR_2, RogueHub_Text_NameMart
	goto RogueHub_EventScript_Construction
RogueHub_EventScript_ConstructionHealer::
	bufferstring STR_VAR_2, RogueHub_Text_NameHealer
	goto RogueHub_EventScript_Construction
RogueHub_EventScript_ConstructionTutor::
	bufferstring STR_VAR_2, RogueHub_Text_NameTutor
	goto RogueHub_EventScript_Construction
RogueHub_EventScript_ConstructionAbilityLab::
	bufferstring STR_VAR_2, RogueHub_Text_NameAbilityLab
	goto RogueHub_EventScript_Construction
RogueHub_EventScript_ConstructionSupply::
	bufferstring STR_VAR_2, RogueHub_Text_NameSupply
	goto RogueHub_EventScript_Construction
RogueHub_EventScript_ConstructionDraft::
	bufferstring STR_VAR_2, RogueHub_Text_NameDraft
RogueHub_EventScript_Construction:
	lock
	faceplayer
	msgbox RogueHub_Text_Construction, MSGBOX_DEFAULT
	release
	end

@ ---------------------------------------------------------------------------
@ Areas

RogueHub_EventScript_Mart::
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemart RogueHub_Mart_Items
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
RogueHub_Mart_Items:
	.2byte ITEM_POKE_BALL
	.2byte ITEM_GREAT_BALL
	.2byte ITEM_ULTRA_BALL
	.2byte ITEM_POTION
	.2byte ITEM_SUPER_POTION
	.2byte ITEM_HYPER_POTION
	.2byte ITEM_FULL_HEAL
	.2byte ITEM_REVIVE
	.2byte ITEM_ETHER
	.2byte ITEM_FIRE_STONE
	.2byte ITEM_WATER_STONE
	.2byte ITEM_THUNDER_STONE
	.2byte ITEM_LEAF_STONE
	.2byte ITEM_MOON_STONE
	pokemartlistend

RogueHub_EventScript_Healer::
	lock
	faceplayer
	msgbox RogueHub_Text_HealerAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, RogueHub_EventScript_HealerBye
	closemessage
	fadescreen FADE_TO_BLACK
	playfanfare MUS_HEAL
	special HealPlayerParty
	waitfanfare
	fadescreen FADE_FROM_BLACK
	msgbox RogueHub_Text_HealerDone, MSGBOX_DEFAULT
	release
	end
RogueHub_EventScript_HealerBye:
	msgbox RogueHub_Text_HealerBye, MSGBOX_DEFAULT
	release
	end

RogueHub_EventScript_Tutor::
	lock
	faceplayer
	msgbox RogueHub_Text_TutorIntro, MSGBOX_DEFAULT
RogueHub_EventScript_TutorChooseMon:
	msgbox RogueHub_Text_TutorWhichMon, MSGBOX_DEFAULT
	setmoverelearnerstate MOVE_RELEARNER_LEVEL_UP_MOVES
	chooseboxmon SELECT_PC_MON_MOVE_RELEARNER
	waitstate
	goto_if_eq VAR_0x8004, PARTY_NOTHING_CHOSEN, RogueHub_EventScript_TutorBye
	special IsSelectedMonEgg
	goto_if_eq VAR_RESULT, TRUE, RogueHub_EventScript_TutorEgg
	special Special_HasMoveToRelearn
	goto_if_eq VAR_RESULT, FALSE, RogueHub_EventScript_TutorNoMoves
	msgbox RogueHub_Text_TutorWhichMove, MSGBOX_DEFAULT
	special TeachMoveRelearnerMove
	goto RogueHub_EventScript_TutorChooseMon
RogueHub_EventScript_TutorEgg:
	msgbox RogueHub_Text_TutorEgg, MSGBOX_DEFAULT
	goto RogueHub_EventScript_TutorChooseMon
RogueHub_EventScript_TutorNoMoves:
	msgbox RogueHub_Text_TutorNoMoves, MSGBOX_DEFAULT
	goto RogueHub_EventScript_TutorChooseMon
RogueHub_EventScript_TutorBye:
	msgbox RogueHub_Text_TutorBye, MSGBOX_DEFAULT
	release
	end

RogueHub_EventScript_AbilityLab::
	lock
	faceplayer
	msgbox RogueHub_Text_AbilityLabIntro, MSGBOX_DEFAULT
	special ChoosePartyMon
	waitstate
	goto_if_ge VAR_0x8004, PARTY_SIZE, RogueHub_EventScript_AbilityLabBye
	special IsSelectedMonEgg
	goto_if_eq VAR_RESULT, TRUE, RogueHub_EventScript_AbilityLabEgg
	special Roguelite_BuildAbilityMenu
	message RogueHub_Text_AbilityLabWhich
	waitmessage
	dynmultistack 0, 0, TRUE, 3, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	special Roguelite_SetMonAbility
	goto_if_eq VAR_RESULT, FALSE, RogueHub_EventScript_AbilityLabBye
	playse SE_EXP_MAX
	msgbox RogueHub_Text_AbilityLabDone, MSGBOX_DEFAULT
	release
	end
RogueHub_EventScript_AbilityLabEgg:
	msgbox RogueHub_Text_AbilityLabEgg, MSGBOX_DEFAULT
RogueHub_EventScript_AbilityLabBye:
	msgbox RogueHub_Text_AbilityLabBye, MSGBOX_DEFAULT
	release
	end

RogueHub_EventScript_Supply::
	lock
	faceplayer
	buffernumberstring STR_VAR_1, VAR_ROGUE_HUB_SUPPLY_LEVEL
	msgbox RogueHub_Text_Supply, MSGBOX_DEFAULT
	release
	end

RogueHub_EventScript_Draft::
	lock
	faceplayer
	buffernumberstring STR_VAR_1, VAR_ROGUE_HUB_DRAFT_LEVEL
	msgbox RogueHub_Text_Draft, MSGBOX_DEFAULT
	release
	end

RogueHub_EventScript_Storage::
	lock
	faceplayer
	msgbox RogueHub_Text_Storage, MSGBOX_DEFAULT
	special ShowPokemonStorageSystemPC
	release
	end

RogueHub_EventScript_Sign::
	special Roguelite_BufferCoins
	buffernumberstring STR_VAR_2, VAR_ROGUELITE_BEST_WAVE
	msgbox RogueHub_Text_Sign, MSGBOX_SIGN
	end

@ ---------------------------------------------------------------------------
@ Text

RogueHub_Text_NameMart:
	.string "Poké Mart$"
RogueHub_Text_NameHealer:
	.string "Healer$"
RogueHub_Text_NameTutor:
	.string "Move Tutor$"
RogueHub_Text_NameAbilityLab:
	.string "Ability Lab$"
RogueHub_Text_NameSupply:
	.string "Supply Depot$"
RogueHub_Text_NameDraft:
	.string "Draft Office$"

RogueHub_Text_Sign:
	.string "ROGUE HUB\n"
	.string "Coins: {STR_VAR_1}   Best wave: {STR_VAR_2}$"

RogueHub_Text_BuilderIntro:
	.string "I'm the Builder. Bring me Rogue Coins\n"
	.string "from your runs and I'll build the hub\l"
	.string "up for you.$"

RogueHub_Text_BuilderPrompt:
	.string "Rogue Coins: {STR_VAR_1}.\n"
	.string "What should I build?$"

RogueHub_Text_Built:
	.string "Done! The {STR_VAR_2} is open.\n"
	.string "Coins left: {STR_VAR_1}.$"

RogueHub_Text_TooPoor:
	.string "You need more Rogue Coins for\n"
	.string "the {STR_VAR_2}.$"

RogueHub_Text_Maxed:
	.string "The {STR_VAR_2} can't be upgraded\n"
	.string "any further.$"

RogueHub_Text_BuilderBye:
	.string "Come back when you've got coins.$"

RogueHub_Text_Construction:
	.string "This is where the {STR_VAR_2}\n"
	.string "will go. Ask the Builder to put it up.$"

RogueHub_Text_HealerAsk:
	.string "Shall I heal your Pokémon?$"

RogueHub_Text_HealerDone:
	.string "Your Pokémon are fighting fit!$"

RogueHub_Text_HealerBye:
	.string "Take care out there.$"

RogueHub_Text_TutorIntro:
	.string "I can remind your Pokémon of moves\n"
	.string "they could have learned. Free of charge.$"

RogueHub_Text_TutorWhichMon:
	.string "Which Pokémon?$"

RogueHub_Text_TutorWhichMove:
	.string "Which move should it remember?$"

RogueHub_Text_TutorEgg:
	.string "An Egg can't learn moves.$"

RogueHub_Text_TutorNoMoves:
	.string "There's nothing new I can teach\n"
	.string "that one.$"

RogueHub_Text_TutorBye:
	.string "Drop by any time.$"

RogueHub_Text_AbilityLabIntro:
	.string "Every Pokémon has a regular ability\n"
	.string "plus three innates that are always on.\p"
	.string "I can switch which regular ability\n"
	.string "one of yours uses.$"

RogueHub_Text_AbilityLabWhich:
	.string "Which ability for {STR_VAR_1}?$"

RogueHub_Text_AbilityLabDone:
	.string "{STR_VAR_1}'s ability is now\n"
	.string "{STR_VAR_2}!$"

RogueHub_Text_AbilityLabEgg:
	.string "I can't work on an Egg.$"

RogueHub_Text_AbilityLabBye:
	.string "Come back any time.$"

RogueHub_Text_Supply:
	.string "Supply Depot, level {STR_VAR_1}.\n"
	.string "Every run starts with extra items:\p"
	.string "Lv1: 5 Poké Balls, 3 Super Potions\n"
	.string "Lv2: 5 Great Balls, 2 Revives\l"
	.string "Lv3: 3 Rare Candies, 1 Full Restore$"

RogueHub_Text_Draft:
	.string "Draft Office, level {STR_VAR_1}.\n"
	.string "Each level adds 2 points to spend\l"
	.string "when you draft a team.$"

RogueHub_Text_Storage:
	.string "I look after the Pokémon Storage\n"
	.string "System. Here you go.$"
