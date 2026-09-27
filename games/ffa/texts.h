// Part I dialogue (docs/part1.md §7, the original English, lightly re-punctuated).
// '\1' = the hero's name; lines are wrapped at run time (dialog.c), '\n' forces a break.
// Speakers: the name tag above the box (0 = none).
#ifndef FFA_TEXTS_H
#define FFA_TEXTS_H

enum { SP_NONE, SP_HERO, SP_EDOUARD, SP_OLEN, SP_LARC, SP_JESS, SP_VOICE, SP_VILLAGER, SP_SELLER, SP_GUARD,
       SP_PRISONER, NSPEAKER };

enum {
    T_LOCKED, T_WEAPON_ROOM,
    // room 8
    T_STORY1, T_SLEEP, T_PLAQUE8, T_FOUND_POTION, T_LARC_DOOR,
    // room 6
    T_SWORD_Q, T_SWORD_A, T_ED_SWORD, T_ED_CLOSED,
    T_C1, T_C2, T_C3, T_C4, T_C5, T_C6, T_C7, T_C8, T_C9, T_C10, T_C11, T_C12, T_C13, T_C14, T_C15, T_C16,
    T_C17, T_C18, T_C19, T_C20, T_C21, T_C22, T_C23,
    // room 5, 4, 7, 18
    T_SOLDIER5, T_SELLER, T_MESSAGE4, T_BOOK_EXCALIBUR, T_BOOK_LANGUAGE, T_BOOK_CLOUD, T_BOOK_WAR,
    T_OLEN_KEY, T_FOUND_DKEY, T_OLEN_CURE, T_FOUND_CURE, T_OLEN_LUCK, T_OLEN_COURAGE, T_JESS, T_CARROTS,
    // dungeon
    T_HOLDS_KEY, T_FOUND_LKEY, T_NOTICE, T_FOUND_ETHER, T_SWITCH_Q, T_LOCK_NOISE, T_TRAP, T_FOUND_ANTIDOTE,
    T_RIDDLE, T_RIGHT, T_FALSE, T_FOUND_FIRE, T_CELLS_SIGN, T_PLAQUE15, T_HOLDS_SOMETHING, T_FOUND_WRIST,
    T_FOUND_SWORD, T_GOLD_SEAL, T_BOSS1, T_BOSS2, T_BOSS3, T_FOUND_CELL2, T_CELL2, T_FOUND_BANGLE,
    T_NOTHING, T_SHOP_POOR, T_SHOP_BYE,
    NTEXT
};

static const char *const speaker_name[NSPEAKER] = {
    0, "\1", "Edouard", "Olen", "Larc", "Jess", "A voice", "Villager", "Seller", "Soldier", "Prisoner"
};

typedef struct { u8 speaker; const char *s; } Text;

static const Text texts[NTEXT] = {
    { SP_NONE, "The door is locked." },
    { SP_NONE, "The Weapon Room is not behind this door, anyway..." },
    // room 8
    { SP_EDOUARD, "My son... as you know, today is a great day for you: you'll be named KNIGHT. As soon as you're ready, come downstairs." },
    { SP_NONE, "Sleep?" },
    { SP_NONE, "\1 STRIFE, born in this Castle, son of Edouard STRIFE and Larc's brother." },
    { SP_NONE, "Found 1 Potion!" },
    { SP_HERO, "My brother Larc's room. He always locks it before leaving." },
    // room 6
    { SP_HERO, "Father, I don't see where my sword is..." },
    { SP_EDOUARD, "Your sword! Oh, that's right! It is in the dungeon, in the weapon room. It's a ritual: each future Knight must beat some monsters to find his sword... So good luck!" },
    { SP_EDOUARD, "Your sword is in the dungeon." },
    { SP_EDOUARD, "Closed? Olen is the last who went there..." },
    { SP_EDOUARD, "You found your sword, \1..." },
    { SP_HERO, "Yes... and I was attacked by a prisoner, but nothing important." },
    { SP_EDOUARD, "Strange... Ha! Here are Olen and Jess! Only Larc is missing..." },
    { SP_EDOUARD, "It's him... You were at the village?" },
    { SP_LARC, "Yes... You're also here, \1..." },
    { SP_EDOUARD, "Well. Let's start, \1..." },
    { SP_EDOUARD, "I, Edouard STRIFE, Lord of Milunia Kingdom, declare \1 STRIFE, my son, Knight. You must defend your castle and your people. Now swear respect and obedience to your Lord." },
    { SP_HERO, "I swear it..." },
    { SP_EDOUARD, "\1, you are now a Milunia Knight!" },
    { SP_JESS, "Long life the Knight \1!!!" },
    { SP_OLEN, "Long life the Lord STRIFE!!!" },
    { SP_EDOUARD, "Thank you, my friends. Now, I want to speak with my sons..." },
    { SP_JESS, "Yes, my Lord..." },
    { SP_LARC, "Father... I have to go to the village again..." },
    { SP_EDOUARD, "But... well... Go. ...but that's not polite to leave like that!" },
    { SP_EDOUARD, "\1..." },
    { SP_VILLAGER, "My Lord! My Lord! The village's been attacked again, some Chocobos from the Ranch've been killed! We need help!" },
    { SP_EDOUARD, "Strange... Don't worry, go tell the Village's Chief we're going to help him." },
    { SP_VILLAGER, "All right. Thanks, my Lord!" },
    { SP_EDOUARD, "Well... \1. I want you to help Milunia Village: it's your first mission." },
    { SP_HERO, "Yes, I'll go." },
    { SP_EDOUARD, "I could have asked your brother for some help, but he's just an incompetent! Well, good luck, Knight \1!!" },
    { SP_EDOUARD, "By the way... don't go to BRAMANA, we have no good relations with them nowadays." },
    // room 5, 4, 7, 18
    { SP_GUARD, "I love this Castle: I feel safe, behind these solid walls..." },
    { SP_SELLER, "Hello Sir \1, I'm the \"official\" potion seller in the Castle." },
    { SP_NONE, "Message: We left to play at the Chocodome!" },
    { SP_NONE, "EXCALIBUR is the Legendary Sword with an unbelievable Power. It's kept somewhere in this castle. It seems you must know the Ancient Language to get it." },
    { SP_NONE, "The Ancient Language has disappeared, but it seems some scholars still know it..." },
    { SP_NONE, "Cloud embodies the strength and the power of the Strifes. He saved the world by killing SEPHIROTH, 1000 years ago." },
    { SP_NONE, "Since the 30 years War, BRAMANA kingdom is the potential enemy of Milunia. Yet, the two kingdoms got well along until that outstanding war." },
    { SP_OLEN, "\1! How are you! Ready for the Big Day! Here is the Dungeon Key... Good luck!" },
    { SP_NONE, "Found: Dungeon Key!" },
    { SP_OLEN, "Take this, \1, it will be useful." },
    { SP_NONE, "Found: Materia Cure!" },
    { SP_OLEN, "Good luck!" },
    { SP_OLEN, "\1, you mustn't get discouraged!" },
    { SP_JESS, "A lot of prisoners' corpses remain in the dungeon... It scares me so much!" },
    { SP_NONE, "Fresh carrots..." },
    // dungeon
    { SP_HERO, "It holds a key." },
    { SP_NONE, "Found: little Key!" },
    { SP_NONE, "The Thirty Years War was very deadly: there were \2 injured. Among them, 3000 died." },
    { SP_NONE, "Found 1 Ether!" },
    { SP_NONE, "Push the switch?" },
    { SP_NONE, "There's a lock noise." },
    { SP_HERO, "Oh no!..." },
    { SP_NONE, "Found 1 Antidote!" },
    { SP_NONE, "This door will open if you know the number of injured who stayed alive." },
    { SP_NONE, "That's right... You can enter." },
    { SP_NONE, "False." },
    { SP_NONE, "Found: Materia Fire!" },
    { SP_NONE, "In front of you, the prison cells." },
    { SP_NONE, "John BARNARD\nCondemned to Death Sentence." },
    { SP_HERO, "It holds something..." },
    { SP_NONE, "Found: Accessory Power Wrist!" },
    { SP_NONE, "Found: Weapon Buster Sword!" },
    { SP_NONE, "Locked by Gold Seal." },
    { SP_VOICE, "Ya there... What're ya doing here?" },
    { SP_HERO, "I ask you the same question." },
    { SP_PRISONER, "I'm sent by... well, I'm a prisoner, because of your father! You'll pay for him!" },
    { SP_NONE, "Found: Cell 2 Key!" },
    { SP_HERO, "Cell 2! This prisoner must have come from there..." },
    { SP_NONE, "Found: Armor Bronze Bangle!" },
    { SP_NONE, "Nothing special." },
    { SP_SELLER, "Sorry, not enough gils." },
    { SP_SELLER, "See you later." },
};

#endif
