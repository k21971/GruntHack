/*	SCCS Id: @(#)mhitu.c	3.4	2003/11/26	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "artifact.h"

STATIC_VAR NEARDATA struct obj *otmp;

STATIC_DCL void FDECL(urustm, (struct monst *, struct obj *));
# ifdef OVL1
STATIC_DCL boolean FDECL(u_slip_free, (struct monst *,struct attack *));
STATIC_DCL int FDECL(passiveum, (struct permonst *,struct monst *,struct attack *));
# endif /* OVL1 */

#ifdef OVLB
# ifdef SEDUCE
STATIC_DCL void FDECL(mayberem, (struct obj *, const char *));
# endif
#endif /* OVLB */

STATIC_DCL boolean FDECL(diseasemu, (struct monst *));
STATIC_DCL int FDECL(hitmu, (struct monst *,struct attack *));
STATIC_DCL int FDECL(gulpmu, (struct monst *,struct attack *));
STATIC_DCL int FDECL(explmu, (struct monst *,struct attack *,BOOLEAN_P));
STATIC_DCL void FDECL(missmu,(struct monst *,int,struct attack *));
STATIC_DCL void FDECL(mswings,(struct monst *,struct obj *));
STATIC_DCL void FDECL(wildmiss, (struct monst *,struct attack *));

STATIC_DCL void FDECL(hurtarmor,(int));
STATIC_DCL void FDECL(hitmsg,(struct monst *,struct attack *));

/* See comment in mhitm.c.  If we use this a lot it probably should be */
/* changed to a parameter to mhitu. */
static int dieroll;

#ifdef OVL1

const char *
weaphitmsg(obj,uhitm)
struct obj *obj;
boolean uhitm;
{
       /* lucern hammers and bec-de-corbins both whack and pierce */
       return ((objects[obj->otyp].oc_dir & WHACK &&
               (!(objects[obj->otyp].oc_dir & PIERCE) || rn2(2))) ?
                       ((objects[obj->otyp].oc_skill == P_CLUB ||
                       objects[obj->otyp].oc_skill == P_MACE ||
                       objects[obj->otyp].oc_skill == P_MORNING_STAR) ?
                           "club" : "whack") :
               (objects[obj->otyp].oc_dir & PIERCE &&
               (!(objects[obj->otyp].oc_dir & SLASH) || rn2(2))) ?
                       (is_blade(obj) ? "stab" : "jab") :
               (objects[obj->otyp].oc_dir & SLASH) ?
                       (uhitm && Role_if(PM_BARBARIAN) ? "smite" :
                        rn2(2) ? "hack" : is_axe(obj) ? "chop" : "slash") :
               (objects[obj->otyp].oc_skill == P_WHIP) ?
                       "whip" :
               "hit");
}

const char *
barehitmsg(mtmp)
struct monst *mtmp;
{
       if (!strcmp(mbodypart(mtmp, HAND),"claw") ||
	   !strcmp(mbodypart(mtmp, HAND),"paw") ||
               !strcmp(mbodypart(mtmp, HAND),"foreclaw") || is_bird(mtmp->data))
                return "claw";
       if (!strcmp(mbodypart(mtmp, HAND),"swirl") || /* elementals */
           !strcmp(mbodypart(mtmp, HAND),"tentacle")) { /* krakens */
               if (mtmp->data == &mons[PM_EARTH_ELEMENTAL])
                   return "pummel";
               return "lash";
       } if (is_undead(mtmp->data))
               return "scratch";
       if (mtmp->data == &mons[PM_MONK] || mtmp->data == &mons[PM_SAMURAI]
               || (martial_bonus() &&
                  (mtmp == &youmonst ||
                   /* Assumes monk or samurai quest monsters */
                  mtmp->data->msound == MS_LEADER ||
                  mtmp->data->msound == MS_GUARDIAN ||
                  mtmp->data->msound == MS_NEMESIS)))
               return "strike";
       if (mtmp->data == &mons[PM_NURSE])
               return "touch";
       return "punch";
}

STATIC_OVL void
hitmsg(mtmp, mattk)
register struct monst *mtmp;
register struct attack *mattk;
{
	int compat;
	char buf[BUFSZ];

	/* Note: if opposite gender, "seductively" */
	/* If same gender, "engagingly" for nymph, normal msg for others */
	if((compat = could_seduce(mtmp, &youmonst, mattk))
			&& !mtmp->mcan && !mtmp->mspec_used) {
		pline("%s %s you %s.", Monnam(mtmp),
			Blind ? "talks to" : "smiles at",
			compat == 2 ? "engagingly" : "seductively");
	} else
	    switch (mattk->aatyp) {
		case AT_BITE:
                        pline("%s %ss!", Monnam(mtmp), has_beak(mtmp->data) ?
                                "peck" : "bite");
			break;
		case AT_KICK:
			pline("%s kicks%c", Monnam(mtmp),
				    thick_skinned(youmonst.data) ? '.' : '!');
			break;
		case AT_STNG:
			pline("%s stings!", Monnam(mtmp));
			break;
		case AT_BUTT:
			pline("%s butts!", Monnam(mtmp));
			break;
		case AT_TUCH:
			pline("%s touches you!", Monnam(mtmp));
			break;
		case AT_TENT:
			pline("%s tentacles suck you!",
				        s_suffix(Monnam(mtmp)));
			break;
		case AT_EXPL:
		case AT_BOOM:
			pline("%s explodes!", Monnam(mtmp));
			break;
                case AT_WEAP:
                        if (MON_WEP(mtmp)) {
			   Strcpy(buf, weaphitmsg(MON_WEP(mtmp),FALSE));
                            if (is_launcher(MON_WEP(mtmp)) ||
                                is_missile(MON_WEP(mtmp)) ||
                                is_ammo(MON_WEP(mtmp)) ||
                                is_pole(MON_WEP(mtmp)))
                                    Sprintf(buf,"hit");
                            pline("%s %s%s!", Monnam(mtmp),
                               makeplural(buf), !strcmp(buf,"whip") ||
	 					!strcmp(buf,"hack") ||
	 					!strcmp(buf,"chop") ||
	 					!strcmp(buf,"jab") ? " you" : "");
                            break;
                        } /*fallthrough*/
                case AT_CLAW:
		       Strcpy(buf, barehitmsg(mtmp));
                        pline("%s %s%s!", Monnam(mtmp), makeplural(buf),
	 					!strcmp(buf,"scratch") ||
	 					!strcmp(buf,"strike") ||
	 					!strcmp(buf,"punch") ? "" : " you");
                        break;
		default:
			pline("%s hits!", Monnam(mtmp));
	    }
}

STATIC_OVL void
missmu(mtmp, nearmiss, mattk)		/* monster missed you */
register struct monst *mtmp;
register int nearmiss;
register struct attack *mattk;
{

	char buf[BUFSZ];

	if (!canspotmon(mtmp))
	    map_invisible(mtmp->mx, mtmp->my);

	if(could_seduce(mtmp, &youmonst, mattk) && !mtmp->mcan)
	    pline("%s pretends to be friendly.", Monnam(mtmp));
	else {
	    if (!flags.verbose)
		pline("%s misses.", Monnam(mtmp));
	    else
	    {
	        int tmp = mons[u.umonnum].ac + mtmp->m_lev + 10,
		    piece = 0;
	        if(multi < 0) tmp += 4;
	        if((Invis && !sees_invis(mtmp)) || !mtmp->mcansee)
		    tmp -= 2;
	        if(mtmp->mtrapped) tmp -= 2;
	        if(tmp <= 0) tmp = 1;
	
		while (tmp > nearmiss)
		{
		    piece++;
		    switch(piece)
		    {
			case 1:
	                    tmp -= u.uspellprot;
			    Sprintf(buf, "golden haze");
			    break;
			case 2:
	                    if (HProtection & INTRINSIC)
			        tmp -= u.ublessed,
				sprintf(buf, "divine protection");
			    break;
			case 3:
	                    if(uleft && uleft->otyp == RIN_PROTECTION)
			        tmp -= uleft->spe,
				Sprintf(buf, "%s", xname(uleft));
			    break;
			case 4:
	                    if(uright && uright->otyp == RIN_PROTECTION)
			        tmp -= uright->spe,
				Sprintf(buf, "%s", xname(uright));
			    break;
			case 5:
	                    if (uarms) tmp -= ARM_BONUS(uarms),
			               Sprintf(buf, "%s", xname(uarms));
			    break;
			case 6:
	                    if(uarmc) tmp -= ARM_BONUS(uarmc),
			              Sprintf(buf, "%s", xname(uarmc));
			    break;
		        case 7:
	                    if(uarm) tmp -= ARM_BONUS(uarm),
			             Sprintf(buf, "%s", xname(uarm));
			    break;
			case 8:
	                    if(uarmh) tmp -= ARM_BONUS(uarmh),
			              Sprintf(buf, "%s", xname(uarmh));
			    break;
			case 9:
#ifdef TOURIST
	                    if(uarmu) tmp -= ARM_BONUS(uarmu),
			              Sprintf(buf, "%s", xname(uarmu));
#endif
			case 10:
	                    if(uarmg) tmp -= ARM_BONUS(uarmg),
			              Sprintf(buf, "%s", xname(uarmg));
			    break;
			case 11:
	                    if(uarmf) tmp -= ARM_BONUS(uarmf),
			              Sprintf(buf, "%s", xname(uarmf));
			    break;
			    break;
			default: break;
		    }
		    if (piece > 11) break;
		}
		if (piece > 0 && piece <= 11)
		    pline("%s %s is %sstopped by your %s!",
		          s_suffix(Monnam(mtmp)),
		          mpoisons_subj(mtmp, mattk),
			  (tmp == nearmiss) ? "narrowly " 
		                            : "", buf);
		else if (piece == 0)
		    pline("%s just misses!", Monnam(mtmp));
		else
		    pline("%s misses.", Monnam(mtmp));
	    }
	}
	stop_occupation();
}

STATIC_OVL void
mswings(mtmp, otemp)		/* monster swings obj */
register struct monst *mtmp;
register struct obj *otemp;
{
	if (!flags.verbose || Blind || !mon_visible(mtmp))
		return;
	pline("%s %s %s %s.", Monnam(mtmp),
	      (objects[otemp->otyp].oc_dir & PIERCE) ? "thrusts" : "swings",
	      mhis(mtmp), singular(otemp, xname));
}

/* return how a poison attack was delivered */
const char *
mpoisons_subj(mtmp, mattk)
struct monst *mtmp;
struct attack *mattk;
{
	if (mattk->aatyp == AT_WEAP) {
	    struct obj *mwep = (mtmp == &youmonst) ? uwep : MON_WEP(mtmp);
	    /* "Foo's attack was poisoned." is pretty lame, but at least
	       it's better than "sting" when not a stinging attack... */
	    return (!mwep || !mwep->opoisoned) ? "attack" : "weapon";
	} else {
	    return (mattk->aatyp == AT_TUCH) ? "contact" :
		   (mattk->aatyp == AT_GAZE) ? "gaze" :
		   (mattk->aatyp == AT_BITE) ? "bite" : 
		   (mattk->aatyp == AT_STNG) ? "sting" :
		                               "attack";
	}
}

/* called when your intrinsic speed is taken away */
void
u_slow_down()
{
	HFast = 0L;
	if (!Fast)
	    You("slow down.");
	else	/* speed boots */
	    Your("quickness feels less natural.");
	exercise(A_DEX, FALSE);
}

#endif /* OVL1 */
#ifdef OVLB

STATIC_OVL void
wildmiss(mtmp, mattk)		/* monster attacked your displaced image */
	register struct monst *mtmp;
	register struct attack *mattk;
{
	int compat;

	/* no map_invisible() -- no way to tell where _this_ is coming from */

	if (!flags.verbose) return;
	if (!cansee(mtmp->mx, mtmp->my)) return;
		/* maybe it's attacking an image around the corner? */

	compat = (mattk->adtyp == AD_SEDU || mattk->adtyp == AD_SSEX) &&
		 could_seduce(mtmp, &youmonst, (struct attack *)0);

	if (!mtmp->mcansee || (Invis && !sees_invis(mtmp))) {
	    const char *swings =
		mattk->aatyp == AT_BITE ? "snaps" :
		mattk->aatyp == AT_KICK ? "kicks" :
		(mattk->aatyp == AT_STNG ||
		 mattk->aatyp == AT_BUTT ||
		 nolimbs(mtmp->data)) ? "lunges" : "swings";

	    if (compat)
		pline("%s tries to touch you and misses!", Monnam(mtmp));
	    else
		switch(rn2(3)) {
		case 0: pline("%s %s wildly and misses!", Monnam(mtmp),
			      swings);
		    break;
		case 1: pline("%s attacks a spot beside you.", Monnam(mtmp));
		    break;
		case 2: pline("%s strikes at %s!", Monnam(mtmp),
				levl[mtmp->mux][mtmp->muy].typ == WATER
				    ? "empty water" : "thin air");
		    break;
		default:pline("%s %s wildly!", Monnam(mtmp), swings);
		    break;
		}

	} else if (Displaced) {
	    if (compat)
		pline("%s smiles %s at your %sdisplaced image...",
			Monnam(mtmp),
			compat == 2 ? "engagingly" : "seductively",
			Invis ? "invisible " : "");
	    else
		pline("%s strikes at your %sdisplaced image and misses you!",
			/* Note: if you're both invisible and displaced,
			 * only monsters which see invisible will attack your
			 * displaced image, since the displaced image is also
			 * invisible.
			 */
			Monnam(mtmp),
			Invis ? "invisible " : "");

	} else if (Underwater) {
	    /* monsters may miss especially on water level where
	       bubbles shake the player here and there */
	    if (compat)
		pline("%s reaches towards your distorted image.",Monnam(mtmp));
	    else
		pline("%s is fooled by water reflections and misses!",Monnam(mtmp));

	} else impossible("%s attacks you without knowing your location?",
		Monnam(mtmp));
}

void
expels(mtmp, mdat, message)
register struct monst *mtmp;
register struct permonst *mdat; /* if mtmp is polymorphed, mdat != mtmp->data */
boolean message;
{
	if (message) {
		if (is_animal(mdat))
			You("get regurgitated!");
		else {
			char blast[40];
			register int i;

			blast[0] = '\0';
			for(i = 0; i < NATTK; i++)
				if(mdat->mattk[i].aatyp == AT_ENGL)
					break;
			if (mdat->mattk[i].aatyp != AT_ENGL)
			      impossible("Swallower has no engulfing attack?");
			else {
				if (is_whirly(mdat)) {
					switch (mdat->mattk[i].adtyp) {
						case AD_ELEC:
							Strcpy(blast,
						      " in a shower of sparks");
							break;
						case AD_COLD:
							Strcpy(blast,
							" in a blast of frost");
							break;
					}
				} else
					Strcpy(blast, " with a squelch");
				You("get expelled from %s%s!",
				    mon_nam(mtmp), blast);
			}
		}
	}
	unstuck(mtmp, mdat);	/* ball&chain returned in unstuck() */
	mnexto(mtmp);
	newsym(u.ux,u.uy);
	spoteffects(TRUE);
	/* to cover for a case where mtmp is not in a next square */
	if(um_dist(mtmp->mx,mtmp->my,1))
		pline("Brrooaa...  You land hard at some distance.");
}

#endif /* OVLB */
#ifdef OVL0

/* select a monster's next attack, possibly substituting for its usual one */
struct attack *
getmattk(mptr, indx, prev_result, alt_attk_buf)
struct permonst *mptr;
int indx, prev_result[];
struct attack *alt_attk_buf;
{
    struct attack *attk = &mptr->mattk[indx];

    /* prevent a monster with two consecutive disease or hunger attacks
       from hitting with both of them on the same turn; if the first has
       already hit, switch to a stun attack for the second */
    if (indx > 0 && prev_result[indx - 1] > 0 &&
	    (attk->adtyp == AD_DISE ||
		attk->adtyp == AD_PEST ||
		attk->adtyp == AD_FAMN) &&
	    attk->adtyp == mptr->mattk[indx - 1].adtyp) {
	*alt_attk_buf = *attk;
	attk = alt_attk_buf;
	attk->adtyp = AD_STUN;
    }
    return attk;
}

/*
 * mattacku: monster attacks you
 *	return values are the same as mattackm 
 *	Note: if you're displaced or invisible the monster might attack the
 *		wrong position...
 *	Assumption: it's attacking you or an empty square; if there's another
 *		monster which it attacks by mistake, the caller had better
 *		take care of it...
 */
int
mattacku(mtmp)
	register struct monst *mtmp;
{
	struct	attack	*mattk, alt_attk;
	int	i, j, res = 0, tmp, sum[NATTK];
	struct	permonst *mdat = mtmp->data;
	boolean ranged = (distu(mtmp->mx, mtmp->my) > 3);
		/* Is it near you?  Affects your actions */
	boolean range2 = !monnear(mtmp, mtmp->mux, mtmp->muy);
		/* Does it think it's near you?  Affects its actions */
	boolean foundyou = (mtmp->mux==u.ux && mtmp->muy==u.uy);
		/* Is it attacking you or your image? */
	boolean youseeit = canseemon(mtmp);
		/* Might be attacking your image around the corner, or
		 * invisible, or you might be blind....
		 */
	
	char buf[BUFSZ];
	
	if(!ranged) nomul(0);
	if(mtmp->mhp <= 0 || (Underwater && !is_swimmer(mtmp->data)))
	    return(MM_MISS);

	/* If swallowed, can only be affected by u.ustuck */
	if(u.uswallow) {
	    if(mtmp != u.ustuck)
		return(MM_MISS);
	    u.ustuck->mux = u.ux;
	    u.ustuck->muy = u.uy;
	    range2 = 0;
	    foundyou = 1;
	    if(u.uinvulnerable) return (MM_MISS); /* stomachs can't hurt you! */
	}

#ifdef STEED
	else if (u.usteed) {
		if (mtmp == u.usteed)
			/* Your steed won't attack you */
			return (MM_MISS);
		/* Orcs like to steal and eat horses and the like */
		if (!rn2(is_orc(mtmp) ? 2 : 4) &&
				distu(mtmp->mx, mtmp->my) <= 2) {
			/* Attack your steed instead */
			i = mattackm(mtmp, u.usteed);
			j = i & ~(MM_DEF_DIED);
			return(j); 
		}
	}
#endif

	if (u.uundetected && !range2 && foundyou && !u.uswallow) {
		u.uundetected = 0;
		if (is_hider(youmonst.data)) {
		    coord cc; /* maybe we need a unexto() function? */
		    struct obj *obj;

                    if (youmonst.data == &mons[PM_TRAPPER])
		        You("ambush %s!", mon_nam(mtmp));
		    else
		        You("fall from the %s!", ceiling(u.ux,u.uy));
		    if (enexto(&cc, u.ux, u.uy, youmonst.data)) {
			remove_monster(mtmp->mx, mtmp->my);
			remove_monster_img(mtmp->mix, mtmp->miy);
			newsym(mtmp->mx,mtmp->my);
			newsym(mtmp->mix,mtmp->miy);
			place_monster(mtmp, u.ux, u.uy);
			if(mtmp->wormno) worm_move(mtmp);
			teleds(cc.x, cc.y, TRUE);
			set_apparxy(mtmp);
			newsym(u.ux,u.uy);
		    } else {
		        /* this is a pathological case, so might as well
			 * be silly about it... */
                        if (youmonst.data == &mons[PM_TRAPPER])
			    pline("%s dies from the shock of the ambush.",
			          Monnam(mtmp));
			else
			    pline("%s is killed by a falling %s (you)!",
					Monnam(mtmp), youmonst.data->mname);
			killed(mtmp, AD_PHYS);
			newsym(u.ux,u.uy);
			if (mtmp->mhp > 0) return(MM_MOVED|MM_MISS);
			else return(MM_MOVED|MM_AGR_DIED);
		    }
		    if (youmonst.data->mlet != S_PIERCER)
			return(MM_MISS);	/* trappers don't attack */

		    obj = which_armor(mtmp, WORN_HELMET);
		    if (obj && is_metallic(obj)) {
			Your("blow glances off %s %s.",
			               s_suffix(mon_nam(mtmp)),
				       helmet_name(obj));
		    } else {
			if (3 + find_mac(mtmp) <= rnd(20)) {
			    pline("%s is hit by a falling piercer (you)!",
								Monnam(mtmp));
			    if ((mtmp->mhp -= d(3,6)) < 1)
				killed(mtmp, AD_PHYS);
			} else
			  pline("%s is almost hit by a falling piercer (you)!",
								Monnam(mtmp));
		    }
		} else {
		    if (!youseeit)
			pline("It tries to move where you are hiding.");
		    else {
			/* Ugly kludge for eggs.  The message is phrased so as
			 * to be directed at the monster, not the player,
			 * which makes "laid by you" wrong.  For the
			 * parallelism to work, we can't rephrase it, so we
			 * zap the "laid by you" momentarily instead.
			 */
			struct obj *obj = level.objects[u.ux][u.uy];

			if (obj ||
			      (youmonst.data->mlet == S_EEL && is_pool(u.ux, u.uy))) {
			    int save_spe = 0; /* suppress warning */
			    if (obj) {
				save_spe = obj->spe;
				if (obj->otyp == EGG) obj->spe = 0;
			    }
			    if (youmonst.data->mlet == S_EEL)
		pline("Wait, %s!  There's a hidden %s named %s there!",
				m_monnam(mtmp), youmonst.data->mname, plname);
			    else
	     pline("Wait, %s!  There's a %s named %s hiding under %s!",
				m_monnam(mtmp), youmonst.data->mname, plname,
				doname(level.objects[u.ux][u.uy]));
			    if (obj) obj->spe = save_spe;
			} else
			    impossible("hiding under nothing?");
		    }
		    newsym(u.ux,u.uy);
		}
		return(MM_MOVED|MM_MISS);
	}
	if (youmonst.data->mlet == S_MIMIC && youmonst.m_ap_type &&
		    !range2 && foundyou && !u.uswallow) {
		if (!youseeit) pline("It gets stuck on you.");
		else pline("Wait, %s!  That's a %s named %s!",
			   m_monnam(mtmp), youmonst.data->mname, plname);
		u.ustuck = mtmp;
		youmonst.m_ap_type = M_AP_NOTHING;
		youmonst.mappearance = 0;
		newsym(u.ux,u.uy);
		return(MM_MOVED|MM_MISS);
	}

	/* player might be mimicking an object */
	if (youmonst.m_ap_type == M_AP_OBJECT && !range2 && foundyou && !u.uswallow) {
	    if (!youseeit)
		 pline("%s %s!", Something,
			(likes_gold(mtmp->data) && youmonst.mappearance == GOLD_PIECE) ?
			"tries to pick you up" : "disturbs you");
	    else pline("Wait, %s!  That %s is really %s named %s!",
			m_monnam(mtmp),
			mimic_obj_name(&youmonst),
			an(mons[u.umonnum].mname),
			plname);
	    if (multi < 0) {	/* this should always be the case */
		Sprintf(buf, "You appear to be %s again.",
			Upolyd ? (const char *) an(youmonst.data->mname) :
			    (const char *) "yourself");
		unmul(buf);	/* immediately stop mimicking */
	    }
	    return(MM_MOVED|MM_MISS);
	}

/*	Work out the armor class differential	*/
	tmp = AC_VALUE(u.uac) + 10;		/* tmp ~= 0 - 20 */
	tmp += mtmp->m_lev;
	if(multi < 0) tmp += 4;
	if((Invis && !sees_invis(mtmp)) || !mtmp->mcansee)
		tmp -= 2;
	if(mtmp->mtrapped) tmp -= 2;
	if(tmp <= 0) tmp = 1;

	/* make eels visible the moment they hit/miss us */
	if(mdat->mlet == S_EEL && mtmp->minvis && cansee(mtmp->mx,mtmp->my)) {
		mtmp->minvis = 0;
		newsym(mtmp->mx,mtmp->my);
	}

/*	Special demon handling code */
	if(!mtmp->cham && is_demon(mdat) && !range2
	   && mtmp->data != &mons[PM_BALROG]
	   && mtmp->data != &mons[PM_SUCCUBUS]
	   && mtmp->data != &mons[PM_INCUBUS]
           && mtmp->data != &mons[PM_NIGHTMARE])
	    if(!mtmp->mcan && !rn2(13))	msummon(mtmp);

/*	Special lycanthrope handling code */
	if(!mtmp->cham && is_were(mdat) && !range2) {

	    if(is_human(mtmp)) {
		if(!rn2(5 - (night() * 2)) && !mtmp->mcan) new_were(mtmp);
	    } else if(!rn2(30) && !mtmp->mcan) new_were(mtmp);
	    mdat = mtmp->data;

	    if(!rn2(10) && !mtmp->mcan) {
	    	int numseen, numhelp;
		char genericwere[BUFSZ];

		Strcpy(genericwere, "creature");
		numhelp = were_summon(mdat, FALSE, &numseen, genericwere);
		if (youseeit) {
			pline("%s summons help!", Monnam(mtmp));
			if (numhelp > 0) {
			    if (numseen == 0)
				You_feel("hemmed in.");
			} else pline("But none comes.");
		} else {
			const char *from_nowhere;

			if (flags.soundok) {
				pline("%s %s!", Something,
					makeplural(growl_sound(mtmp)));
				from_nowhere = "";
			} else from_nowhere = " from nowhere";
			if (numhelp > 0) {
			    if (numseen < 1) You_feel("hemmed in.");
			    else {
				if (numseen == 1)
			    		Sprintf(buf, "%s appears",
							an(genericwere));
			    	else
			    		Sprintf(buf, "%s appear",
							makeplural(genericwere));
				pline("%s%s!", upstart(buf), from_nowhere);
			    }
			} /* else no help came; but you didn't know it tried */
		}
	    }
	}

	if(u.uburied) {
	    return(MM_MISS);
	}

	if(u.uinvulnerable) {
	    /* monsters won't attack you */

	    /* only print messages for monsters that actually can attack */
	    for(i = 0; i < NATTK; i++) {
	        if (mtmp->data->mattk[i].aatyp != AT_NONE &&
		    mtmp->data->mattk[i].aatyp != AT_BOOM)
		    break;
	    }
	    if (i == NATTK) return (MM_MOVED|MM_MISS);

	    if(mtmp == u.ustuck)
		pline("%s loosens its grip slightly.", Monnam(mtmp));
	    else if(!range2) {
		if (youseeit || sensemon(mtmp))
		    pline("%s starts to attack you, but pulls back.",
			  Monnam(mtmp));
		else
		    You_feel("%s move nearby.", something);
	    }
	    return (MM_MOVED|MM_MISS);
	}

	/* Unlike defensive stuff, don't let them use item _and_ attack. */
	if(find_offensive(mtmp)) {
		int foo = use_offensive(mtmp);

		if (foo != 0) return (foo == 1 ?
		                      MM_MOVED|MM_HIT|MM_AGR_DIED :
				      MM_MOVED|MM_HIT);
	}

	for(i = 0; i < NATTK; i++) {

	    sum[i] = 0;
	    mattk = getmattk(mdat, i, sum, &alt_attk);
	    if (u.uswallow && (mattk->aatyp != AT_ENGL))
		continue;
	    switch(mattk->aatyp) {
		case AT_CLAW:	/* "hand to hand" attacks */
		case AT_KICK:
		case AT_BITE:
		case AT_STNG:
		case AT_TUCH:
		case AT_BUTT:
		case AT_TENT:
			if(!range2 && (!MON_WEP(mtmp) || mtmp->mconf || Conflict ||
					!touch_petrifies(youmonst.data))) {
			    res |= MM_MOVED;
			    if (foundyou) {
				if(tmp > (j = dieroll = rnd(20+i))) {
				    if (mattk->aatyp != AT_KICK ||
					    !thick_skinned(youmonst.data))
					sum[i] = hitmu(mtmp, mattk);
				} else
				    missmu(mtmp, j, mattk);
			    } else
				wildmiss(mtmp, mattk);
			}
			break;

		case AT_HUGS:	/* automatic if prev two attacks succeed */
			/* Note: if displaced, prev attacks never succeeded */
			if((!range2 && i>=2 && sum[i-1] && sum[i-2])
							|| mtmp == u.ustuck)
			{
				res |= MM_MOVED;
				sum[i]= hitmu(mtmp, mattk);
			}
			break;

		case AT_GAZE:	/* can affect you either ranged or not */
			/* Medusa gaze already operated through m_respond in
			 * dochug(); don't gaze more than once per round.
			 */
			if (mdat != &mons[PM_MEDUSA])
				sum[i] = gazemu(mtmp, mattk);
			break;

		case AT_EXPL:	/* automatic hit if next to, and aimed at you */
			res |= MM_MOVED;
			if(!range2) sum[i] = explmu(mtmp, mattk, foundyou);
			break;

		case AT_ENGL:
			if (!range2) {
			    res |= MM_MOVED;
			    if(foundyou) {
				if(u.uswallow || tmp > (j = rnd(20+i))) {
				    /* Force swallowing monster to be
				     * displayed even when player is
				     * moving away */
				    flush_screen(1);
				    sum[i] = gulpmu(mtmp, mattk);
				} else {
				    missmu(mtmp, j, mattk);
				}
			    } else if (is_animal(mtmp->data)) {
				pline("%s gulps some air!", Monnam(mtmp));
			    } else {
				if (youseeit)
				    pline("%s lunges forward and recoils!",
					  Monnam(mtmp));
				else
				    You_hear("a %s nearby.",
					     is_whirly(mtmp->data) ?
						"rushing noise" : "splat");
			   }
			}
			break;
		case AT_BREA:
			if(range2) {
			    res |= MM_MOVED;
			    sum[i] = breamu(mtmp, mattk);
			}
			/* Note: breamu takes care of displacement */
			break;
		case AT_SPIT:
			if(range2) {
			    res |= MM_MOVED;
			    sum[i] = spitmu(mtmp, mattk);
			}
			/* Note: spitmu takes care of displacement */
			break;
		case AT_WEAP:
			if(range2) {
#ifdef REINCARNATION
				if (!Is_rogue_level(&u.uz))
#endif
					if ((!select_hwep(mtmp) ||
					     dist2(mtmp->mx, mtmp->my, mtmp->mux, mtmp->muy) > 8) &&
					    thrwmu(mtmp))
						res |= MM_MOVED;
			} else {
			    int hittmp = 0;

			    /* Rare but not impossible.  Normally the monster
			     * wields when 2 spaces away, but it can be
			     * teleported or whatever....
			     */
			    if ((mtmp->weapon_check == NEED_WEAPON ||
				!MON_WEP(mtmp) ||
				(is_launcher(MON_WEP(mtmp)) &&
				 !(MON_WEP(mtmp))->cursed))) {
				mtmp->weapon_check = NEED_HTH_WEAPON;
				/* mon_wield_item resets weapon_check as
				 * appropriate */
				if (mon_wield_item(mtmp, FALSE) != 0) {
				    res |= MM_MOVED;
				    break;
				}
			    }
			    if ((!MON_WEP(mtmp)) ||
			         (is_launcher(MON_WEP(mtmp)))) {
			        /* implies we could not find a HTH weapon */
				if (thrwmu(mtmp)) {
				    res |= MM_MOVED;
				    break;
				}
			    }
			
			    res |= MM_MOVED;
			    if (foundyou) {
				otmp = MON_WEP(mtmp);
				if(otmp) {
				    hittmp = hitval(otmp, &youmonst);
				    tmp += hittmp;
				    mswings(mtmp, otmp);
				}
				if(tmp > (j = dieroll = rnd(20+i)))
				    sum[i] = hitmu(mtmp, mattk);
				else
				    missmu(mtmp, j, mattk);
				/* KMH -- Don't accumulate to-hit bonuses */
				if (otmp)
					tmp -= hittmp;
			    } else
				wildmiss(mtmp, mattk);
			}
			break;
		case AT_MAGC:
			if (range2)
			    sum[i] = buzzmu(mtmp, mattk);
			else {
			    if (foundyou)
				sum[i] = castmu(mtmp, mattk, TRUE, TRUE);
			    else
				sum[i] = castmu(mtmp, mattk, TRUE, FALSE);
			}
			if (sum[i] > 0)
			    res |= MM_MOVED;
			break;

		default:		/* no attack */
			break;
	    }
	    if(flags.botl) bot();
	/* give player a chance of waking up before dying -kaa */
	    if(sum[i] == 1) {	    /* successful attack */
	        res |= MM_HIT;
		if (u.usleep && u.usleep < monstermoves && !rn2(10)) {
		    multi = -1;
		    nomovemsg = "The combat suddenly awakens you.";
		}
	    }
	    if(sum[i] == 2) {
	    	return res |= MM_AGR_DIED;		/* attacker dead */
	    }
	    if(sum[i] == 3) break;  /* attacker teleported, no more attacks */
	    /* sum[i] == 0: unsuccessful attack */
	}
	return res;
}

#endif /* OVL0 */
#ifdef OVLB

/*
 * helper function for some compilers that have trouble with hitmu
 */

STATIC_OVL void
hurtarmor(attk)
int attk;
{
	int	hurt;

	switch(attk) {
	    /* 0 is burning, which we should never be called with */
	    case AD_RUST: hurt = 1; break;
	    case AD_CORR: hurt = 3; break;
	    default: hurt = 2; break;
	}

	/* What the following code does: it keeps looping until it
	 * finds a target for the rust monster.
	 * Head, feet, etc... not covered by metal, or covered by
	 * rusty metal, are not targets.  However, your body always
	 * is, no matter what covers it.
	 */
	while (1) {
	    switch(rn2(5)) {
	    case 0:
		if (!uarmh || !rust_dmg(uarmh, xname(uarmh), hurt, FALSE, &youmonst))
			continue;
		break;
	    case 1:
		if (uarmc) {
		    (void)rust_dmg(uarmc, xname(uarmc), hurt, TRUE, &youmonst);
		    break;
		}
		/* Note the difference between break and continue;
		 * break means it was hit and didn't rust; continue
		 * means it wasn't a target and though it didn't rust
		 * something else did.
		 */
		if (uarm)
		    (void)rust_dmg(uarm, xname(uarm), hurt, TRUE, &youmonst);
#ifdef TOURIST
		else if (uarmu)
		    (void)rust_dmg(uarmu, xname(uarmu), hurt, TRUE, &youmonst);
#endif
		break;
	    case 2:
		if (!uarms || !rust_dmg(uarms, xname(uarms), hurt, FALSE, &youmonst))
		    continue;
		break;
	    case 3:
		if (!uarmg || !rust_dmg(uarmg, xname(uarmg), hurt, FALSE, &youmonst))
		    continue;
		break;
	    case 4:
		if (!uarmf || !rust_dmg(uarmf, xname(uarmf), hurt, FALSE, &youmonst))
		    continue;
		break;
	    }
	    break; /* Out of while loop */
	}
}

#endif /* OVLB */
#ifdef OVL1

STATIC_OVL boolean
diseasemu(mtmp)
struct monst *mtmp;
{
	if (Sick_resistance) {
		You_feel("a slight illness.");
		return FALSE;
	} else {
		make_sick(Sick ? Sick/3L + 1L : (long)rn1(ACURR(A_CON), 20),
			done_in_name(mtmp), TRUE,
			mtmp->data == &mons[PM_ZOMBIE] ||
                        mtmp->data == &mons[PM_DRAGON_ZOMBIE]
			? SICK_ZOMBIE : SICK_NONVOMITABLE);
		return TRUE;
	}
}

/* check whether slippery clothing protects from hug or wrap attack */
STATIC_OVL boolean
u_slip_free(mtmp, mattk)
struct monst *mtmp;
struct attack *mattk;
{
	struct obj *obj = (uarmc ? uarmc : uarm);

#ifdef TOURIST
	if (!obj) obj = uarmu;
#endif
	if (mattk->adtyp == AD_DRIN) obj = uarmh;

	/* if your cloak/armor is greased, monster slips off; this
	   protection might fail (33% chance) when the armor is cursed */
	if (obj && (obj->greased || obj->otyp == OILSKIN_CLOAK ||
	            (obj->oprops & ITEM_OILSKIN)) &&
		(!obj->cursed || rn2(3))) {
	    pline("%s %s your %s %s!",
		  Monnam(mtmp),
		  (mattk->adtyp == AD_WRAP) ?
			"slips off of" : "grabs you, but cannot hold onto",
		  obj->greased ? "greased" : "slippery",
		  /* avoid "slippery slippery cloak"
		     for undiscovered oilskin cloak */
		  (obj->greased || objects[obj->otyp].oc_name_known) ?
			xname(obj) : cloak_simple_name(obj));

	    if (obj->greased && !rn2(2)) {
		pline_The("grease wears off.");
		obj->greased = 0;
		update_inventory();
	    }
	    return TRUE;
	}
	return FALSE;
}

/* armor that sufficiently covers the body might be able to block magic */
int
magic_negation(mon)
struct monst *mon;
{
	struct obj *armor;
	int armpro = 0;

	armor = (mon == &youmonst) ? uarm : which_armor(mon, W_ARM);
	if (armor && armpro < objects[armor->otyp].a_can)
	    armpro = objects[armor->otyp].a_can;
	armor = (mon == &youmonst) ? uarmc : which_armor(mon, W_ARMC);
	if (armor && armpro < objects[armor->otyp].a_can)
	    armpro = objects[armor->otyp].a_can;
	armor = (mon == &youmonst) ? uarmh : which_armor(mon, W_ARMH);
	if (armor && armpro < objects[armor->otyp].a_can)
	    armpro = objects[armor->otyp].a_can;

	/* armor types for shirt, gloves, shoes, and shield don't currently
	   provide any magic cancellation but we might as well be complete */
#ifdef TOURIST
	armor = (mon == &youmonst) ? uarmu : which_armor(mon, W_ARMU);
	if (armor && armpro < objects[armor->otyp].a_can)
	    armpro = objects[armor->otyp].a_can;
#endif
	armor = (mon == &youmonst) ? uarmg : which_armor(mon, W_ARMG);
	if (armor && armpro < objects[armor->otyp].a_can)
	    armpro = objects[armor->otyp].a_can;
	armor = (mon == &youmonst) ? uarmf : which_armor(mon, W_ARMF);
	if (armor && armpro < objects[armor->otyp].a_can)
	    armpro = objects[armor->otyp].a_can;
	armor = (mon == &youmonst) ? uarms : which_armor(mon, W_ARMS);
	if (armor && armpro < objects[armor->otyp].a_can)
	    armpro = objects[armor->otyp].a_can;

#ifdef STEED
	/* this one is really a stretch... */
	armor = (mon == &youmonst) ? 0 : which_armor(mon, W_SADDLE);
	if (armor && armpro < objects[armor->otyp].a_can)
	    armpro = objects[armor->otyp].a_can;
#endif

	return armpro;
}

extern const char * const behead_msg[];

/*
 * hitmu: monster hits you
 *	  returns 2 if monster dies (e.g. "yellow light"), 1 otherwise
 *	  3 if the monster lives but teleported/paralyzed, so it can't keep
 *	       attacking you
 */
STATIC_OVL int
hitmu(mtmp, mattk)
	register struct monst *mtmp;
	register struct attack  *mattk;
{
	register struct permonst *mdat = mtmp->data;
	register int uncancelled, ptmp;
	int dmg, armpro, permdmg;
	char	 buf[BUFSZ];
	struct permonst *olduasmon = youmonst.data;
	int res;

	boolean showed_msg = FALSE;

	if (!canspotmon(mtmp))
	    map_invisible(mtmp->mx, mtmp->my);

/*	If the monster is undetected & hits you, you should know where
 *	the attack came from.
 */
	if(mtmp->mundetected && (hides_under(mdat) || mdat->mlet == S_EEL)) {
	    mtmp->mundetected = 0;
	    if (!(Blind ? Blind_telepat : Unblind_telepat)) {
		struct obj *obj;
		const char *what;

		if ((obj = level.objects[mtmp->mx][mtmp->my]) != 0) {
		    if (Blind && !obj->dknown)
			what = something;
		    else if (is_pool(mtmp->mx, mtmp->my) && !Underwater)
			what = "the water";
		    else
			what = doname(obj);

		    pline("%s was hidden under %s!", Amonnam(mtmp), what);
		}
		newsym(mtmp->mx, mtmp->my);
	    }
	}

/*	First determine the base damage done */
	dmg = d((int)mattk->damn, (int)mattk->damd);
	if(is_undead(mdat) && midnight())
		dmg += d((int)mattk->damn, (int)mattk->damd); /* extra damage */

/*	Next a cancellation factor	*/
/*	Use uncancelled when the cancellation factor takes into account certain
 *	armor's special magic protection.  Otherwise just use !mtmp->mcan.
 */
	armpro = magic_negation(&youmonst);
	uncancelled = !mtmp->mcan && ((rn2(3) >= armpro) || !rn2(50));

	permdmg = 0;
	if (mattk->aatyp == AT_WEAP && MON_WEP(mtmp))
	    goto physical;
/*	Now, adjust damages via resistances or specific attacks */
top:
	switch(mattk->adtyp) {
	    case AD_PHYS:
physical:
		if (mattk->aatyp == AT_HUGS && !sticks(youmonst.data)) {
		    if(!u.ustuck && rn2(2)) {
			if (u_slip_free(mtmp, mattk)) {
			    dmg = 0;
			} else {
			    u.ustuck = mtmp;
			    pline("%s grabs you!", Monnam(mtmp));
			    showed_msg = TRUE;
			}
		    } else if(u.ustuck == mtmp) {
			exercise(A_STR, FALSE);
			You("are being %s.",
			      (mtmp->data == &mons[PM_ROPE_GOLEM])
			      ? "choked" : "crushed");
			showed_msg = TRUE;
		    }
		} else {			  /* hand to hand weapon */
		    if(mattk->aatyp == AT_WEAP && otmp) {
			if ((otmp->otyp == CORPSE ||
			    (otmp->otyp == ROCK && otmp->corpsenm != 0))
				&& touch_petrifies(&mons[otmp->corpsenm])) {
			    dmg = 1;
			    showed_msg = TRUE;
			    pline("%s hits you with the %s.",
				Monnam(mtmp), xname(otmp));
			    if (!Stoned)
				goto do_stone;
			}
			dmg += dmgval(otmp, &youmonst);
			if (objects[otmp->otyp].oc_material == SILVER &&
			    hates_silver(youmonst.data)) {
			    pline("The silver sears your flesh!");
			}
			if (dmg <= 0) dmg = 1;
			if (!(((otmp->oclass == WEAPON_CLASS/* &&
			        otmp->oprops*/) || otmp->oartifact) &&
				artifact_hit(mtmp, &youmonst, otmp, &dmg,dieroll)))
			     hitmsg(mtmp, mattk);
			showed_msg = TRUE;
			if (!dmg) {
			    if (mattk->aatyp == AT_WEAP &&
			        mattk->adtyp != AD_PHYS &&
				MON_WEP(mtmp))
			    	goto top;
			    break;
			}
			if (u.mh > 1 && u.mh > ((u.uac>0) ? dmg : dmg+u.uac) &&
				   objects[otmp->otyp].oc_skill != WHACK &&
					(u.umonnum==PM_BLACK_PUDDING
					|| u.umonnum==PM_BROWN_PUDDING)) {
			    /* This redundancy necessary because you have to
			     * take the damage _before_ being cloned.
			     */
			    struct monst *mtmp2;
			    if (u.uac < 0) dmg += u.uac;
			    if (dmg < 1) dmg = 1;
			    if (dmg > 1) exercise(A_STR, FALSE);
			    u.mh -= dmg;
			    flags.botl = 1;
			    dmg = 0;
			    if((mtmp2 = cloneu()))
			    {
			        You("divide as %s hits you!",mon_nam(mtmp));
				u.mhmax >>= 1;
				if (u.mh > u.mhmax) u.mh = u.mhmax;
				mtmp2->mhpmax >>= 1;
				if (mtmp2->mhp > mtmp2->mhpmax)
				    mtmp2->mhp = mtmp2->mhpmax;
			    }
			}
			urustm(mtmp, otmp);
		    } else if (mattk->aatyp != AT_TUCH || dmg != 0 ||
				mtmp != u.ustuck) {
			hitmsg(mtmp, mattk);
		    }
		}
		if (mattk->aatyp == AT_WEAP && mattk->adtyp != AD_PHYS &&
		    MON_WEP(mtmp))
		    goto top;
		if (mtmp->data == &mons[PM_WATER_ELEMENTAL])
		{
		    showed_msg = TRUE;
		    goto do_rust;
		}
		break;
	    case AD_DISE:
	    	if (!showed_msg)
		    hitmsg(mtmp, mattk);
		if (!diseasemu(mtmp)) dmg = 0;
		break;
	    case AD_FIRE:
	    	if (!showed_msg)
		    hitmsg(mtmp, mattk);
		if (uncancelled) {
		    pline("You're %s!", on_fire(youmonst.data, mattk));
		    if (youmonst.data == &mons[PM_STRAW_GOLEM] ||
		        youmonst.data == &mons[PM_PAPER_GOLEM]) {
			    You("roast!");
			    u.mh = -1;
			    /* KMH -- this is okay with unchanging */
			    killer_format = NO_KILLER_PREFIX;
			    rehumanize("roasted");
			    break;
		    } else if (Fire_resistance) {
			pline_The("fire doesn't feel hot!");
			dmg = 0;
		    }
		    if((int) mtmp->m_lev > rn2(20))
			destroy_item(SCROLL_CLASS, AD_FIRE);
		    if((int) mtmp->m_lev > rn2(20))
			destroy_item(POTION_CLASS, AD_FIRE);
		    if((int) mtmp->m_lev > rn2(25))
			destroy_item(SPBOOK_CLASS, AD_FIRE);
		    burn_away_slime();
		} else dmg = 0;
		break;
	    case AD_COLD:
	    	if (!showed_msg)
		    hitmsg(mtmp, mattk);
		if (uncancelled) {
		    pline("You're covered in frost!");
		    if (Cold_resistance) {
			pline_The("frost doesn't seem cold!");
			dmg = 0;
		    }
		    if((int) mtmp->m_lev > rn2(20))
			destroy_item(POTION_CLASS, AD_COLD);
		} else dmg = 0;
		break;
	    case AD_ELEC:
	    	if (!showed_msg)
		    hitmsg(mtmp, mattk);
		if (uncancelled) {
		    You("get zapped!");
		    if (Shock_resistance) {
			pline_The("zap doesn't shock you!");
			dmg = 0;
		    }
		    if((int) mtmp->m_lev > rn2(20))
			destroy_item(WAND_CLASS, AD_ELEC);
		    if((int) mtmp->m_lev > rn2(20))
			destroy_item(RING_CLASS, AD_ELEC);
		} else dmg = 0;
		break;
	    case AD_SLEE:
	    	if (!showed_msg)
		    hitmsg(mtmp, mattk);
		if (uncancelled && multi >= 0 && !rn2(5)) {
		    if (Sleep_resistance) break;
		    fall_asleep(-rnd(10), TRUE);
		    if (Blind) You("are put to sleep!");
		    else You("are put to sleep by %s!", mon_nam(mtmp));
		}
		break;
	    case AD_BLND:
		if (can_blnd(mtmp, &youmonst, mattk->aatyp, (struct obj*)0)) {
		    if (!Blind) pline("%s blinds you!", Monnam(mtmp));
		    make_blinded(Blinded+(long)dmg,FALSE);
		    if (!Blind) Your("%s", vision_clears);
		}
		dmg = 0;
		break;
	    case AD_DRST:
		ptmp = A_STR;
		goto dopois;
	    case AD_DRDX:
		ptmp = A_DEX;
		goto dopois;
	    case AD_DRCO:
		ptmp = A_CON;
dopois:
	    	if (!showed_msg)
		    hitmsg(mtmp, mattk);
		if (uncancelled && !rn2(8)) {
		    Sprintf(buf, "%s %s",
			    s_suffix(Monnam(mtmp)), mpoisons_subj(mtmp, mattk));
		    poisoned(buf, ptmp, mdat->mname, 30);
		}
		break;
	    case AD_DRIN:
	    	if (!showed_msg)
		    hitmsg(mtmp, mattk);
		if (defends(AD_DRIN, uwep) || !has_head(youmonst.data)) {
		    You("don't seem harmed.");
		    /* Not clear what to do for green slimes */
		    break;
		}
		if (u_slip_free(mtmp,mattk)) break;

		if ((mtmp->data == &mons[PM_ZOMBIE] || mtmp->data == &mons[PM_DRAGON_ZOMBIE]) && rn2(5))
		{
		    if (uncancelled)
                        diseasemu(mtmp);
		    break;
		}

		if (uarmh && rn2(8)) {
		    /* not body_part(HEAD) */
		    Your("%s blocks the attack to your head.",
		         helmet_name(uarmh));
		    break;
		}
		if (Half_physical_damage) dmg = (dmg+1) / 2;
		mdamageu(mtmp, mattk, dmg);

		if (!uarmh || uarmh->otyp != DUNCE_CAP) {
		    Your("brain is eaten!");
		    /* No such thing as mindless players... */
		    if (ABASE(A_INT) <= ATTRMIN(A_INT)) {
			int lifesaved = 0;
			struct obj *wore_amulet = uamul;

			while(1) {
			    /* avoid looping on "die(y/n)?" */
			    if (lifesaved && (discover || wizard)) {
				if (wore_amulet && !uamul) {
				    /* used up AMULET_OF_LIFE_SAVING; still
				       subject to dying from brainlessness */
				    wore_amulet = 0;
				} else {
				    /* explicitly chose not to die;
				       arbitrarily boost intelligence */
				    ABASE(A_INT) = ATTRMIN(A_INT) + 2;
				    You_feel("like a scarecrow.");
				    break;
				}
			    }

			    if (lifesaved)
				pline("Unfortunately your brain is still gone.");
			    else
				Your("last thought fades away.");
			    killer = "brainlessness";
			    killer_format = KILLED_BY;
			    done(DIED);
			    lifesaved++;
			}
		    }
		}
		/* adjattrib gives dunce cap message when appropriate */
		(void) adjattrib(A_INT, -rnd(2), FALSE);
		forget_levels(25);	/* lose memory of 25% of levels */
		forget_objects(25);	/* lose memory of 25% of objects */
		exercise(A_WIS, FALSE);
		if (mtmp->data == &mons[PM_ZOMBIE] ||
                    mtmp->data == &mons[PM_DRAGON_ZOMBIE])
		    diseasemu(mtmp);
		break;
	    case AD_PLYS:
	    	if (!showed_msg)
		    hitmsg(mtmp, mattk);
		if (uncancelled && multi >= 0 && !rn2(3)) {
		    if (Free_action) {
			You("momentarily stiffen.");            
		    } else {
			if (Blind) You("are frozen!");
			else You("are frozen by %s!", mon_nam(mtmp));
			nomovemsg = 0;	/* default: "you can move again" */
			nomul2(-rnd(10), "paralyzed");
			exercise(A_DEX, FALSE);
		    }
		}
		break;
	    case AD_DRLI:
	    	if (!showed_msg)
		    hitmsg(mtmp, mattk);
		if (uncancelled && !rn2(3) && !Drain_resistance) {
		    Sprintf(killer_buf, "%s life drainage",
		    	    s_suffix(done_in_name(mtmp)));
		    losexp(killer_buf);
		}
		break;
	    case AD_LEGS:
		{ register long side = rn2(2) ? RIGHT_SIDE : LEFT_SIDE;
		  const char *sidestr = (side == RIGHT_SIDE) ? "right" : "left";

		/* This case is too obvious to ignore, but Nethack is not in
		 * general very good at considering height--most short monsters
		 * still _can_ attack you when you're flying or mounted.
		 * [FIXME: why can't a flying attacker overcome this?]
		 */
		  if (
#ifdef STEED
			u.usteed ||
#endif
				    Levitation || Flying) {
		    pline("%s tries to reach your %s %s!", Monnam(mtmp),
			  sidestr, body_part(LEG));
		    dmg = 0;
		  } else if (mtmp->mcan) {
		    pline("%s nuzzles against your %s %s!", Monnam(mtmp),
			  sidestr, body_part(LEG));
		    dmg = 0;
		  } else {
		    if (uarmf) {
			if (rn2(2) && (uarmf->otyp == LOW_BOOTS ||
					     uarmf->otyp == SHOES))
			    pline("%s pricks the exposed part of your %s %s!",
				Monnam(mtmp), sidestr, body_part(LEG));
			else if (!rn2(5))
			    pline("%s pricks through your %s boot!",
				Monnam(mtmp), sidestr);
			else {
			    pline("%s scratches your %s boot!", Monnam(mtmp),
				sidestr);
			    dmg = 0;
			    break;
			}
		    } else pline("%s pricks your %s %s!", Monnam(mtmp),
			  sidestr, body_part(LEG));
		    set_wounded_legs(side, rnd(60-ACURR(A_DEX)));
		    exercise(A_STR, FALSE);
		    exercise(A_DEX, FALSE);
		  }
		  break;
		}
	    case AD_STON:	/* cockatrice */
	    	if (!showed_msg)
		    hitmsg(mtmp, mattk);
		if(!rn2(3)) {
		    if (mtmp->mcan) {
			if (flags.soundok)
			    You_hear("a cough from %s!", mon_nam(mtmp));
		    } else {
			if (flags.soundok)
			    You_hear("%s hissing!", s_suffix(mon_nam(mtmp)));
			if(!rn2(10) ||
			    (flags.moonphase == NEW_MOON && !have_lizard())) {
 do_stone:
			    if (!Stoned && !Stone_resistance
				    && !(poly_when_stoned(youmonst.data) &&
					polymon(PM_STONE_GOLEM))) {
				Stoned = 5;
				Strcpy(u.ustone_cause,
					mtmp->data->mname);
				if (mtmp->data->geno & G_UNIQ) {
				    if (!type_is_pname(mtmp->data)) {
					/* "the" buffer may be reallocated */
					Strcpy(u.ustone_cause,
						the(u.ustone_cause));
				    }
				    u.ustone_fmt =
				    	KILLED_BY;
				} else u.ustone_fmt =
					KILLED_BY_AN;
				return(1);
				/* You("turn to stone..."); */
				/* done_in_by(mtmp); */
			    }
			}
		    }
		}
		break;
	    case AD_STCK:
	    	if (!showed_msg)
		    hitmsg(mtmp, mattk);
		if (uncancelled && !u.ustuck && !sticks(youmonst.data))
			u.ustuck = mtmp;
		break;
	    case AD_WRAP:
		if ((!mtmp->mcan || u.ustuck == mtmp) && !sticks(youmonst.data)) {
		    if (!u.ustuck && !rn2(10)) {
			if (u_slip_free(mtmp, mattk)) {
			    dmg = 0;
			} else {
			    pline("%s swings itself around you!",
				  Monnam(mtmp));
			    u.ustuck = mtmp;
			}
		    } else if(u.ustuck == mtmp) {
			if (is_pool(mtmp->mx,mtmp->my) && !Swimming
			    && !Amphibious) {
			    pline("%s drowns you...", Monnam(mtmp));
			    killer_format = waterbody_prefix(mtmp->mx, mtmp->my);
			    Sprintf(buf, "%s by %s",
			            Is_waterlevel(&u.uz)
				    ? "the Plane of Water"
			            : waterbody_name(mtmp->mx, mtmp->my),
				    an(mtmp->data->mname));
			    killer = buf;
			    done(DROWNING);
			} else if(mattk->aatyp == AT_HUGS)
			    You("are being crushed.");
		    } else {
			dmg = 0;
			if(flags.verbose)
			    pline("%s brushes against your %s.", Monnam(mtmp),
				   body_part(LEG));
		    }
		} else dmg = 0;
		break;
	    case AD_WERE:
	    	if (!showed_msg)
		    hitmsg(mtmp, mattk);
		if (uncancelled && !rn2(4) && u.ulycn == NON_PM &&
			!Protection_from_shape_changers &&
			!defends(AD_WERE,uwep)) {
		    You_feel("feverish.");
		    exercise(A_CON, FALSE);
		    u.ulycn = monsndx(mdat);
		}
		break;
	    case AD_SGLD:
	    	if (!showed_msg)
		    hitmsg(mtmp, mattk);
		if (youmonst.data->mlet == mdat->mlet) break;
		if(!mtmp->mcan) stealgold(mtmp);
		break;

	    case AD_SITM:	/* for now these are the same */
	    case AD_SEDU:
		if (is_animal(mtmp->data)) {
	    	        if (!showed_msg)
			    hitmsg(mtmp, mattk);
			if (mtmp->mcan) break;
			/* Continue below */
		} else if (dmgtype(youmonst.data, AD_SEDU)
#ifdef SEDUCE
			|| dmgtype(youmonst.data, AD_SSEX)
#endif
						) {
			pline("%s %s.", Monnam(mtmp), mtmp->minvent ?
		    "brags about the goods some dungeon explorer provided" :
		    "makes some remarks about how difficult theft is lately");
			if (!tele_restrict(mtmp)) (void) rloc(mtmp, FALSE);
			return 3;
		} else if (mtmp->mcan) {
		    if (!Blind) {
			pline("%s tries to %s you, but you seem %s.",
			    Adjmonnam(mtmp, "plain"),
			    flags.female ? "charm" : "seduce",
			    flags.female ? "unaffected" : "uninterested");
		    }
		    if(rn2(3)) {
			if (!tele_restrict(mtmp)) (void) rloc(mtmp, FALSE);
			return 3;
		    }
		    break;
		}
		buf[0] = '\0';
		switch (steal(mtmp, buf)) {
		  case -1:
			return 2;
		  case 0:
			break;
		  default:
			if (!is_animal(mtmp->data) && !tele_restrict(mtmp))
			    (void) rloc(mtmp, FALSE);
			if (is_animal(mtmp->data) && *buf) {
			    if (canseemon(mtmp))
				pline("%s tries to %s away with %s.",
				      Monnam(mtmp),
				      locomotion(mtmp->data, "run"),
				      buf);
			}
			monflee(mtmp, 0, FALSE, FALSE);
			return 3;
		}
		break;
#ifdef SEDUCE
	    case AD_SSEX:
		if(could_seduce(mtmp, &youmonst, mattk) == 1
			&& !mtmp->mcan)
		    if (doseduce(mtmp))
			return 3;
		break;
#endif
	    case AD_SAMU:
	    	if (!showed_msg)
		    hitmsg(mtmp, mattk);
		/* when the Wiz hits, 1/20 steals the amulet */
		if (u.uhave.amulet ||
		     u.uhave.bell || u.uhave.book || u.uhave.menorah
		     || u.uhave.questart) /* carrying the Quest Artifact */
		    if (!rn2(20)) stealamulet(mtmp);
		break;

	    case AD_TLPT:
	    	if (!showed_msg)
		    hitmsg(mtmp, mattk);
		if (uncancelled) {
		    if(flags.verbose)
			Your("position suddenly seems very uncertain!");
		    tele();
		}
		break;
	    case AD_RUST:
do_rust:
	    	if (!showed_msg)
		    hitmsg(mtmp, mattk);
		if (mtmp->mcan) break;
		if (u.umonnum == PM_IRON_GOLEM) {
			You("rust!");
			u.mh = -1;
			/* KMH -- this is okay with unchanging */
			killer_format = NO_KILLER_PREFIX;
			rehumanize("rusted to pieces");
			break;
		}
		hurtarmor(AD_RUST);
		break;
	    case AD_CORR:
	    	if (!showed_msg)
		    hitmsg(mtmp, mattk);
		if (mtmp->mcan) break;
		hurtarmor(AD_CORR);
		break;
	    case AD_DCAY:
	    	if (!showed_msg)
		    hitmsg(mtmp, mattk);
		if (mtmp->mcan) break;
		if (u.umonnum == PM_WOOD_GOLEM ||
		    u.umonnum == PM_LEATHER_GOLEM) {
			You("rot!");
			u.mh = -1;
			/* KMH -- this is okay with unchanging */
			killer_format = NO_KILLER_PREFIX;
			rehumanize("rotted away");
			break;
		}
		hurtarmor(AD_DCAY);
		break;
	    case AD_HEAL:
		/* a cancelled nurse is just an ordinary monster */
		if (mtmp->mcan) {
	    	    if (!showed_msg)
		        hitmsg(mtmp, mattk);
		    break;
		}
		if(!uwep
#ifdef TOURIST
		   && !uarmu
#endif
		   && !uarm && !uarmh && !uarms && !uarmg && !uarmc && !uarmf) {
		    boolean goaway = FALSE;
		    pline("%s %s you!  (I hope you don't mind.)", Monnam(mtmp), makeplural(barehitmsg(mtmp)));
		    if (Upolyd) {
			u.mh += rnd(7);
			if (!rn2(7)) {
			    /* no upper limit necessary; effect is temporary */
			    u.mhmax++;
			    if (!rn2(13)) goaway = TRUE;
			}
			if (u.mh > u.mhmax) u.mh = u.mhmax;
		    } else {
			u.uhp += rnd(7);
			if (!rn2(7)) {
			    /* hard upper limit via nurse care: 25 * ulevel */
			    if (u.uhpmax < 5 * u.ulevel + d(2 * u.ulevel, 10))
				u.uhpmax++;
			    if (!rn2(13)) goaway = TRUE;
			}
			if (u.uhp > u.uhpmax) u.uhp = u.uhpmax;
		    }
		    if (!rn2(3)) exercise(A_STR, TRUE);
		    if (!rn2(3)) exercise(A_CON, TRUE);
		    if (Sick) make_sick(0L, (char *) 0, FALSE, SICK_ALL);
		    flags.botl = 1;
		    if (goaway) {
			mongone(mtmp);
			return 2;
		    } else if (!rn2(33)) {
			if (!tele_restrict(mtmp)) (void) rloc(mtmp, FALSE);
			monflee(mtmp, d(3, 6), TRUE, FALSE);
			return 3;
		    }
		    dmg = 0;
		    passiveum(olduasmon, mtmp, mattk);
		} else {
		    if (Role_if(PM_HEALER)) {
			if (flags.soundok && !(moves % 5))
		      verbalize("Doc, I can't help you unless you cooperate.");
			dmg = 0;
		    } else if (!showed_msg) hitmsg(mtmp, mattk);
		}
		break;
	    case AD_CURS:
	    	if (!showed_msg)
		    hitmsg(mtmp, mattk);
		if(!night() && mdat == &mons[PM_GREMLIN]) break;
		if(!mtmp->mcan && !rn2(10)) {
		    if (flags.soundok) {
			if (Blind) You_hear("laughter.");
			else       pline("%s chuckles.", Monnam(mtmp));
		    }
		    if (u.umonnum == PM_CLAY_GOLEM) {
			pline("Some writing vanishes from your head!");
			u.mh = -1;
			/* KMH -- this is okay with unchanging */
			killer_format = NO_KILLER_PREFIX;
			rehumanize("cancelled");
			break;
		    }
		    attrcurse();
		}
		break;
	    case AD_STUN:
	    	if (!showed_msg)
		    hitmsg(mtmp, mattk);
		if(!mtmp->mcan && !rn2(4)) {
		    make_stunned(HStun + dmg, TRUE);
		    dmg /= 2;
		}
		break;
	    case AD_ACID:
	    	if (!showed_msg)
		    hitmsg(mtmp, mattk);
		if (!mtmp->mcan && !rn2(3))
		    if (Acid_resistance) {
			pline("You're covered in acid, but it seems harmless.");
			dmg = 0;
		    } else {
			pline("You're covered in acid!	It burns!");
			exercise(A_STR, FALSE);
		    }
		else		dmg = 0;
		break;
	    case AD_SLOW:
	    	if (!showed_msg)
		    hitmsg(mtmp, mattk);
		if (uncancelled && HFast &&
					!defends(AD_SLOW, uwep) && !rn2(4))
		    u_slow_down();
		break;
	    case AD_DREN:
	    	if (!showed_msg)
		    hitmsg(mtmp, mattk);
		if (uncancelled && !rn2(4))
		    drain_en(dmg);
		dmg = 0;
		break;
	    case AD_CONF:
	    	if (!showed_msg)
		    hitmsg(mtmp, mattk);
		if(!mtmp->mcan && !rn2(4) && !mtmp->mspec_used) {
		    mtmp->mspec_used = mtmp->mspec_used + (dmg + rn2(6));
		    if(Confusion)
			 You("are getting even more confused.");
		    else You("are getting confused.");
		    make_confused(HConfusion + dmg, FALSE);
		}
		dmg = 0;
		break;
	    case AD_DETH:
		pline("%s reaches out with its deadly touch.", Monnam(mtmp));
		if (is_undead(youmonst.data)) {
		    /* Still does normal damage */
		    pline("Was that the touch of death?");
		    break;
		}
		switch (rn2(20)) {
		case 19: case 18: case 17:
		    if (!Antimagic) {
			killer_format = KILLED_BY_AN;
			killer = "touch of death";
			done(DIED);
			dmg = 0;
			break;
		    } /* else FALLTHRU */
		default: /* case 16: ... case 5: */
		    You_feel("your life force draining away...");
		    permdmg = 1;	/* actual damage done below */
		    break;
		case 4: case 3: case 2: case 1: case 0:
		    if (Antimagic) shieldeff(u.ux, u.uy);
		    pline("Lucky for you, it didn't work!");
		    dmg = 0;
		    break;
		}
		break;
	    case AD_PEST:
		pline("%s reaches out, and you feel fever and chills.",
			Monnam(mtmp));
		(void) diseasemu(mtmp); /* plus the normal damage */
		break;
	    case AD_FAMN:
		pline("%s reaches out, and your body shrivels.",
			Monnam(mtmp));
		exercise(A_CON, FALSE);
		if (!is_fainted()) morehungry(rn1(40,40));
		/* plus the normal damage */
		break;
	    case AD_SLIM:    
	    	if (!showed_msg)
		    hitmsg(mtmp, mattk);
		if (!uncancelled) break;
		if (flaming(youmonst.data) ||
		    levl[u.ux][u.uy].typ == LAVAPOOL) {
		    pline_The("slime burns away!");
		    dmg = 0;
		} else if (Unchanging || unsolid(youmonst.data) ||
				youmonst.data == &mons[PM_GREEN_SLIME]) {
		    You("are unaffected.");
		    dmg = 0;
		} else if (!Slimed) {
		    You("don't feel very well.");
		    if (!Slimed) {
		    	u.uslime_fmt = KILLED_BY;
		    	Strcpy(u.uslime_cause, done_in_name(mtmp));
		    }
		    Slimed = 10L;
		    flags.botl = 1;
		} else
		    pline("Yuck!");
		break;
	    case AD_ENCH:	/* KMH -- remove enchantment (disenchanter) */
	    	if (!showed_msg)
		    hitmsg(mtmp, mattk);
		/* uncancelled is sufficient enough; please
		   don't make this attack less frequent */
		if (uncancelled) {
		    struct obj *obj = some_armor(&youmonst);

		    if (drain_item(obj)) {
			Your("%s less effective.", aobjnam(obj, "seem"));
		    }
		}
		break;
	    case AD_BHED:	/* beheading attack (vorpal jabberwock) */
	        if (dieroll == 1 ||
		    youmonst.data->mlet == S_JABBERWOCK)
		{
		    if (!has_head(youmonst.data)) {
			pline("Somehow, %s misses you wildly.", mon_nam(mtmp));
			dmg = 0;
			break;
		    }
		    if ((noncorporeal(youmonst.data)) ||
		        (amorphous(youmonst.data))) {
			pline("%s slices through your %s.",
			      Monnam(mtmp), body_part(NECK));
			break;
		    }
		    dmg = 2 * (Upolyd ? u.mh : u.uhp)
				  + 200; /*FATAL_DAMAGE_MODIFIER;*/
		    pline(behead_msg[rn2(2)],
			  Monnam(mtmp), "you");
		}
		else if (!showed_msg) hitmsg(mtmp, mattk);
	        break;
	    default:	dmg = 0;
			break;
	}
	if(u.uhp < 1) done_in_by(mtmp, mattk);

/*	Negative armor class reduces damage done instead of fully protecting
 *	against hits.
 */
	if (dmg && u.uac < 0) {
		dmg -= rnd(-u.uac);
		if (dmg < 1) dmg = 1;
	}

	if(dmg) {
	    if (Half_physical_damage
					/* Mitre of Holiness */
		|| (Role_if(PM_PRIEST) && uarmh && is_quest_artifact(uarmh) &&
		    (is_undead(mtmp->data) || is_demon(mtmp->data))))
		dmg = (dmg+1) / 2;

	    if (permdmg) {	/* Death's life force drain */
		int lowerlimit, *hpmax_p;
		/*
		 * Apply some of the damage to permanent hit points:
		 *	polymorphed	    100% against poly'd hpmax
		 *	hpmax > 25*lvl	    100% against normal hpmax
		 *	hpmax > 10*lvl	50..100%
		 *	hpmax >  5*lvl	25..75%
		 *	otherwise	 0..50%
		 * Never reduces hpmax below 1 hit point per level.
		 */
		permdmg = rn2(dmg / 2 + 1);
		if (Upolyd || u.uhpmax > 25 * u.ulevel) permdmg = dmg;
		else if (u.uhpmax > 10 * u.ulevel) permdmg += dmg / 2;
		else if (u.uhpmax > 5 * u.ulevel) permdmg += dmg / 4;

		if (Upolyd) {
		    hpmax_p = &u.mhmax;
		    /* [can't use youmonst.m_lev] */
		    lowerlimit = min((int)youmonst.data->mlevel, u.ulevel);
		} else {
		    hpmax_p = &u.uhpmax;
		    lowerlimit = u.ulevel;
		}
		if (*hpmax_p - permdmg > lowerlimit)
		    *hpmax_p -= permdmg;
		else if (*hpmax_p > lowerlimit)
		    *hpmax_p = lowerlimit;
		else	/* unlikely... */
		    ;	/* already at or below minimum threshold; do nothing */
		flags.botl = 1;
	    }

	    mdamageu(mtmp, mattk, dmg);
	}

	if (dmg)
	    res = passiveum(olduasmon, mtmp, mattk);
	else
	    res = 1;
	stop_occupation();
	return res;
}

#endif /* OVL1 */
#ifdef OVLB

STATIC_OVL int
gulpmu(mtmp, mattk)	/* monster swallows you, or damage if u.uswallow */
	register struct monst *mtmp;
	register struct attack  *mattk;
{
	struct trap *t = t_at(u.ux, u.uy);
	int	tmp = d((int)mattk->damn, (int)mattk->damd);
	int	tim_tmp;
	register struct obj *otmp2;
	int	i;

	if (!u.uswallow) {	/* swallows you */
		if (maybe_polyd(youmonst.data->msize >= MZ_HUGE,
		                Race_if(PM_GIANT))) return(0);
		if ((t && ((t->ttyp == PIT) || (t->ttyp == SPIKED_PIT))) &&
		    sobj_at(BOULDER, u.ux, u.uy))
			return(0);

		if (Punished) unplacebc();	/* ball&chain go away */
		remove_monster(mtmp->mx, mtmp->my);
		remove_monster_img(mtmp->mix, mtmp->miy);
		mtmp->mtrapped = 0;		/* no longer on old trap */
		place_monster(mtmp, u.ux, u.uy);
		u.ustuck = mtmp;
		newsym(mtmp->mx,mtmp->my);
#ifdef STEED
		if (is_animal(mtmp->data) && u.usteed) {
			char buf[BUFSZ];
			/* Too many quirks presently if hero and steed
			 * are swallowed. Pretend purple worms don't
			 * like horses for now :-)
			 */
			Strcpy(buf, mon_nam(u.usteed));
			pline ("%s lunges forward and plucks you off %s!",
				Monnam(mtmp), buf);
			dismount_steed(DISMOUNT_ENGULFED);
		} else
#endif
		pline("%s engulfs you!", Monnam(mtmp));
		stop_occupation();
		reset_occupations();	/* behave as if you had moved */

		if (u.utrap) {
			You("are released from the %s!",
				u.utraptype==TT_WEB ? "web" : "trap");
			u.utrap = 0;
		}

		i = number_leashed();
		if (i > 0) {
		    const char *s = (i > 1) ? "leashes" : "leash";
		    pline_The("%s %s loose.", s, vtense(s, "snap"));
		    unleash_all();
		}

		if (touch_petrifies(youmonst.data) && !resists_ston(mtmp)) {
			expels(mtmp, mtmp->data, FALSE);
			remove_monster(mtmp->mx, mtmp->my);
			remove_monster_img(mtmp->mix, mtmp->miy);
			place_monster(mtmp, u.ux, u.uy);
			if (Punished) placebc();
			minstapetrify(mtmp, TRUE);
			if (mtmp->mhp > 0) return 0;
			else return 2;
		}

		display_nhwindow(WIN_MESSAGE, FALSE);
		vision_recalc(2);	/* hero can't see anything */
		u.uswallow = 1;
		/* u.uswldtim always set > 1 */
		tim_tmp = 25 - (int)mtmp->m_lev;
		if (tim_tmp > 0) tim_tmp = rnd(tim_tmp) / 2;
		else if (tim_tmp < 0) tim_tmp = -(rnd(-tim_tmp) / 2);
		tim_tmp += -u.uac + 10;
		u.uswldtim = (unsigned)((tim_tmp < 2) ? 2 : tim_tmp);
		swallowed(1);
		for (otmp2 = invent; otmp2; otmp2 = otmp2->nobj)
		    (void) snuff_lit(otmp2);
	}

	if (mtmp != u.ustuck) return(0);
	if (u.uswldtim > 0) u.uswldtim -= 1;

	switch(mattk->adtyp) {

		case AD_DGST:
		    if (Slow_digestion) {
			/* Messages are handled below */
			u.uswldtim = 0;
			tmp = 0;
		    } else if (u.uswldtim == 0) {
			pline("%s totally digests you!", Monnam(mtmp));
			tmp = u.uhp;
			if (Half_physical_damage) tmp *= 2; /* sorry */
		    } else {
			pline("%s%s digests you!", Monnam(mtmp),
			      (u.uswldtim == 2) ? " thoroughly" :
			      (u.uswldtim == 1) ? " utterly" : "");
			exercise(A_STR, FALSE);
		    }
		    break;
		case AD_PHYS:
		    if (mtmp->data == &mons[PM_FOG_CLOUD]) {
			You("are laden with moisture and %s",
			    flaming(youmonst.data) ? "are smoldering out!" :
			    Breathless ? "find it mildly uncomfortable." :
			    amphibious(youmonst.data) ? "feel comforted." :
			    "can barely breathe!");
			/* NB: Amphibious includes Breathless */
			if (Amphibious && !flaming(youmonst.data)) tmp = 0;
		    } else {
			You("are pummeled with debris!");
			exercise(A_STR, FALSE);
		    }
		    break;
		case AD_WRAP:
		    /* Why AD_WRAP?  There's no suffocation AD_*,
		     * but WRAP is a rarely used suffocation attack. */
		    if (amphibious(youmonst.data) &&
		        mtmp->data == &mons[PM_WATER_ELEMENTAL])
		    {
		        You("feel quite comfortable in here.");
			tmp = 0;
		    }
		    else if (Breathless)
		    {
		        You("can't breathe, but you don't need to.");
			tmp = 0;
		    }
		    else if (!Strangled)
		    {
		        /*Message: "You can no longer breathe."*/
			pline("It's impossible to breathe in here!");
		        Strangled = 3;
			tmp = 0;
		    }
		    if (mtmp->data == &mons[PM_WATER_ELEMENTAL])
		        water_damage(&youmonst, invent, FALSE, FALSE);
		    break;
		case AD_ACID:
		    if (Acid_resistance) {
			You("are covered with a seemingly harmless goo.");
			tmp = 0;
		    } else {
		      if (Hallucination) pline("Ouch!  You've been slimed!");
		      else You("are covered in slime!  It burns!");
		      exercise(A_STR, FALSE);
		    }
		    break;
		case AD_BLND:
		    if (can_blnd(mtmp, &youmonst, mattk->aatyp, (struct obj*)0)) {
			if(!Blind) {
			    You_cant("see in here!");
			    make_blinded((long)tmp,FALSE);
			    if (!Blind) Your("%s", vision_clears);
			} else
			    /* keep him blind until disgorged */
			    make_blinded(Blinded+1,FALSE);
		    }
		    tmp = 0;
		    break;
		case AD_ELEC:
		    if(!mtmp->mcan && rn2(2)) {
			pline_The("air around you crackles with electricity.");
			if (Shock_resistance) {
				shieldeff(u.ux, u.uy);
				You("seem unhurt.");
				ugolemeffects(AD_ELEC,tmp);
				tmp = 0;
			}
		    } else tmp = 0;
		    break;
		case AD_COLD:
		    if(!mtmp->mcan && rn2(2)) {
			if (Cold_resistance) {
				shieldeff(u.ux, u.uy);
				You_feel("mildly chilly.");
				ugolemeffects(AD_COLD,tmp);
				tmp = 0;
			} else You("are freezing to death!");
		    } else tmp = 0;
		    break;
		case AD_FIRE:
		    if(!mtmp->mcan && rn2(2)) {
			if (Fire_resistance) {
				shieldeff(u.ux, u.uy);
				You_feel("mildly hot.");
				ugolemeffects(AD_FIRE,tmp);
				tmp = 0;
			} else You("are burning to a crisp!");
			burn_away_slime();
		    } else tmp = 0;
		    break;
		case AD_DISE:
		    if (!diseasemu(mtmp)) tmp = 0;
		    break;
	    	case AD_DREN:
		    if (!rn2(4))
		        drain_en(tmp);
		    tmp = 0;
		    break;
		default:
		    tmp = 0;
		    break;
	}

	if (Half_physical_damage) tmp = (tmp+1) / 2;

	mdamageu(mtmp, mattk, tmp);
	if (tmp) stop_occupation();

	if (touch_petrifies(youmonst.data) && !resists_ston(mtmp)) {
	    pline("%s very hurriedly %s you!", Monnam(mtmp),
		  is_animal(mtmp->data)? "regurgitates" : "expels");
	    expels(mtmp, mtmp->data, FALSE);
	} else if (!u.uswldtim || youmonst.data->msize >= MZ_HUGE) {
	    You("get %s!", is_animal(mtmp->data)? "regurgitated" : "expelled");
	    if (flags.verbose && (is_animal(mtmp->data) ||
		    (dmgtype(mtmp->data, AD_DGST) && Slow_digestion)))
		pline("Obviously %s doesn't like your taste.", mon_nam(mtmp));
	    expels(mtmp, mtmp->data, FALSE);
	}
	return(1);
}

STATIC_OVL int
explmu(mtmp, mattk, ufound)	/* monster explodes in your face */
register struct monst *mtmp;
register struct attack  *mattk;
boolean ufound;
{
    if (mtmp->mcan) return(0);

    if (!ufound)
	pline("%s explodes at a spot in %s!",
	    canseemon(mtmp) ? Monnam(mtmp) : "It",
	    levl[mtmp->mux][mtmp->muy].typ == WATER
		? "empty water" : "thin air");
    else {
	register int tmp = /* d((int)mattk->damn, (int)mattk->damd); */
		d((int)(1+mtmp->m_lev/2), (int)mattk->damd);
	register boolean not_affected = defends((int)mattk->adtyp, uwep);

	hitmsg(mtmp, mattk);
	remove_monster(mtmp->mx, mtmp->my);
	remove_monster_img(mtmp->mix, mtmp->miy);
	newsym(mtmp->mx, mtmp->my);

	switch (mattk->adtyp) {
	    case AD_COLD:
		not_affected |= Cold_resistance;
		goto common;
	    case AD_FIRE:
		not_affected |= Fire_resistance;
		goto common;
	    case AD_ELEC:
		not_affected |= Shock_resistance;
common:

		if (!not_affected) {
		    if (ACURR(A_DEX) > rnd(20)) {
			You("duck some of the blast.");
			tmp = (tmp+1) / 2;
		    } else {
		        if (flags.verbose) You("get blasted!");
		    }
		    if (mattk->adtyp == AD_FIRE) burn_away_slime();
		    if (Half_physical_damage) tmp = (tmp+1) / 2;
		    mdamageu(mtmp, mattk, tmp);
		}
		break;

	    case AD_BLND:
		not_affected = resists_blnd(&youmonst);
		if (!not_affected) {
		    /* sometimes you're affected even if it's invisible */
		    if (mon_visible(mtmp) || (rnd(tmp /= 2) > u.ulevel)) {
			You("are blinded by a blast of light!");
			make_blinded((long)tmp, FALSE);
			if (!Blind) Your("%s", vision_clears);
		    } else if (flags.verbose)
			You("get the impression it was not terribly bright.");
		}
		break;

	    case AD_HALU:
		not_affected |= Blind ||
			(u.umonnum == PM_BLACK_LIGHT ||
			 u.umonnum == PM_VIOLET_FUNGUS ||
			 dmgtype(youmonst.data, AD_STUN));
		if (!not_affected) {
		    boolean chg;
		    if (!Hallucination)
			You("are caught in a blast of kaleidoscopic light!");
		    chg = make_hallucinated(HHallucination + (long)tmp,FALSE,0L);
		    You("%s.", chg ? "are freaked out" : "seem unaffected");
		}
		break;

	    default:
		break;
	}
	if (not_affected) {
	    You("seem unaffected by it.");
	    ugolemeffects((int)mattk->adtyp, tmp);
	}
    }
    place_monster(mtmp, mtmp->mx, mtmp->my);
    mondead(mtmp, (int)mattk->adtyp);
    wake_nearto(mtmp->mx, mtmp->my, 7*7);
    if (mtmp->mhp > 0) return(0);
    return(2);	/* it dies */
}

int
gazemu(mtmp, mattk)	/* monster gazes at you */
	register struct monst *mtmp;
	register struct attack  *mattk;
{
	switch(mattk->adtyp) {
	    case AD_STON:
		if (mtmp->mcan || !mtmp->mcansee) {
		    if (!canseemon(mtmp)) break;	/* silently */
		    pline("%s %s.", Monnam(mtmp),
			  (mtmp->data == &mons[PM_MEDUSA] && mtmp->mcan) ?
				"doesn't look all that ugly" :
				"gazes ineffectually");
		    break;
		}
		if (Reflecting && couldsee(mtmp->mx, mtmp->my) &&
			mtmp->data == &mons[PM_MEDUSA]) {
		    /* hero has line of sight to Medusa and she's not blind */
		    boolean useeit = canseemon(mtmp);

		    if (useeit)
			(void) ureflects("%s gaze is reflected by %s %s.",
					 s_suffix(Monnam(mtmp)));
		    if (mon_reflects(mtmp, !useeit ? (char *)0 :
				     "The gaze is reflected away by %s %s!"))
			break;
		    if (!m_canseeu(mtmp)) { /* probably you're invisible */
			if (useeit)
			    pline(
		      "%s doesn't seem to notice that %s gaze was reflected.",
				  Monnam(mtmp), mhis(mtmp));
			break;
		    }
		    if (useeit)
			pline("%s is turned to stone!", Monnam(mtmp));
		    stoned = TRUE;
		    killed(mtmp, AD_STON);

		    if (mtmp->mhp > 0) break;
		    return 2;
		}
		if (canseemon(mtmp) && couldsee(mtmp->mx, mtmp->my) &&
		    !Stone_resistance &&
		    (mtmp->data == &mons[PM_MEDUSA] || !rn2(5))) {
		    You("meet %s gaze.", s_suffix(mon_nam(mtmp)));
		    stop_occupation();
		    if(poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM))
			break;
		    You("turn to stone...");
		    killer_format = KILLED_BY;
		    killer = done_in_name(mtmp); /* mtmp->data->mname; */
		    done(STONING);
		}
		break;
	    case AD_CONF:
		if(!mtmp->mcan && canseemon(mtmp) &&
		   couldsee(mtmp->mx, mtmp->my) &&
		   mtmp->mcansee && !mtmp->mspec_used && rn2(5)) {
		    int conf = d(mattk->damn,mattk->damd);

		    mtmp->mspec_used = mtmp->mspec_used + (conf + rn2(6));
		    if(!Confusion)
			pline("%s gaze confuses you!",
			                  s_suffix(Monnam(mtmp)));
		    else
			You("are getting more and more confused.");
		    make_confused(HConfusion + conf, FALSE);
		    stop_occupation();
		}
		break;
	    case AD_STUN:
		if(!mtmp->mcan && canseemon(mtmp) &&
		   couldsee(mtmp->mx, mtmp->my) &&
		   mtmp->mcansee && !mtmp->mspec_used && rn2(5)) {
		    int stun = d(mattk->damn,mattk->damd);

		    mtmp->mspec_used = mtmp->mspec_used + (stun + rn2(6));
		    pline("%s stares piercingly at you!", Monnam(mtmp));
		    make_stunned(HStun + stun, TRUE);
		    stop_occupation();
		}
		break;
	    case AD_BLND:
		if (!mtmp->mcan && canseemon(mtmp) && !resists_blnd(&youmonst)
			&& distu(mtmp->mx,mtmp->my) <= BOLT_LIM*BOLT_LIM) {
		    int blnd = d((int)mattk->damn, (int)mattk->damd);

		    You("are blinded by %s radiance!",
			              s_suffix(mon_nam(mtmp)));
		    make_blinded((long)blnd,FALSE);
		    stop_occupation();
		    /* not blind at this point implies you're wearing
		       the Eyes of the Overworld; make them block this
		       particular stun attack too */
		    if (!Blind) Your("%s", vision_clears);
		    else make_stunned((long)d(1,3),TRUE);
		}
		break;
	    case AD_FIRE:
		if (!mtmp->mcan && canseemon(mtmp) &&
			couldsee(mtmp->mx, mtmp->my) &&
			mtmp->mcansee && !mtmp->mspec_used && rn2(5)) {
		    int dmg = d(mattk->damn,mattk->damd);

		    pline("%s attacks you with a fiery gaze!", Monnam(mtmp));
		    stop_occupation();
		    if (Fire_resistance) {
			pline_The("fire doesn't feel hot!");
			dmg = 0;
		    }
		    burn_away_slime();
		    if ((int) mtmp->m_lev > rn2(20))
			destroy_item(SCROLL_CLASS, AD_FIRE);
		    if ((int) mtmp->m_lev > rn2(20))
			destroy_item(POTION_CLASS, AD_FIRE);
		    if ((int) mtmp->m_lev > rn2(25))
			destroy_item(SPBOOK_CLASS, AD_FIRE);
		    if (dmg) mdamageu(mtmp, mattk, dmg);
		}
		break;
	    case AD_SLEE:
		if(!mtmp->mcan && canseemon(mtmp) &&
		   couldsee(mtmp->mx, mtmp->my) && mtmp->mcansee &&
		   multi >= 0 && !rn2(5)) {
		   if (Sleep_resistance) {
		    	pline("%s gaze makes you yawn.",
				s_suffix(Monnam(mtmp)));
			stop_occupation();
		   } else {
		   	fall_asleep(-d(mattk->damn,mattk->damd), TRUE);
		    	pline("%s gaze makes you very sleepy...",
				s_suffix(Monnam(mtmp)));
		   }
		}
		break;
	    case AD_SLOW:
		if(!mtmp->mcan && canseemon(mtmp) && mtmp->mcansee &&
		   (HFast & (INTRINSIC|TIMEOUT)) &&
		   !defends(AD_SLOW, uwep) && !rn2(4))
		{
		    pline("You meet %s gaze.", s_suffix(mon_nam(mtmp)));
		    u_slow_down();
		    stop_occupation();
		}
		break;
	    case AD_DISN:
		if(!mtmp->mcan && canseemon(mtmp) &&
		   couldsee(mtmp->mx, mtmp->my) && mtmp->mcansee &&
		   multi >= 0 && !rn2(5)) {
		   if (Disint_resistance) {
		    	pline("You bask in the %s aura of %s gaze.",
				hcolor(NH_BLACK),
				s_suffix(mon_nam(mtmp)));
			stop_occupation();
		   } else {
			pline("%s attacks you with a destructive gaze!",
				Monnam(mtmp));
			if (uarms) {
			    /* destroy shield; other possessions are safe */
			    (void) destroy_arm(uarms);
			    break;
			} else if (uarm) {
			    /* destroy suit; if present, cloak goes too */
			    if (uarmc) (void) destroy_arm(uarmc);
			    (void) destroy_arm(uarm);
			    break;
			}
			/* no shield or suit, you're dead; wipe out cloak
			   and/or shirt in case of life-saving or bones */
			if (uarmc) (void) destroy_arm(uarmc);
#ifdef TOURIST
			if (uarmu) (void) destroy_arm(uarmu);
#endif
	    		u.ugrave_arise = -3; /* don't leave a corpse */

	    		done_in_by(mtmp, mattk);
		   }
		}
	    	break;
	    case AD_CNCL:
	    	if (!mtmp->mcan && canseemon(mtmp) && mtmp->mcansee &&
			!rn2(5))
		{
		    	pline("You meet %s gaze.", s_suffix(mon_nam(mtmp)));
			(void) cancel_monst(&youmonst, (struct obj *)0,
					    FALSE, TRUE, FALSE);
		}
	    	break;
	    default: impossible("Gaze attack %d?", mattk->adtyp);
		break;
	}
	return(0);
}

#endif /* OVLB */
#ifdef OVL1

void
mdamageu(mtmp, mattk, n) /* mtmp hits you with mattk for n points damage */
register struct monst *mtmp;
register struct attack *mattk;
register int n;
{
	flags.botl = 1;
	if (Upolyd) {
		u.mh -= n;
		if (u.mh < 1) rehumanize(l_monnam(mtmp));
	} else {
		u.uhp -= n;
		if(u.uhp < 1) done_in_by(mtmp, mattk);
	}
}

#endif /* OVL1 */
#ifdef OVLB

STATIC_OVL void
urustm(mon, obj)
register struct monst *mon;
register struct obj *obj;
{
	boolean vis;
	boolean is_acid;

	if (!mon || !obj) return; /* just in case */
	if (dmgtype(youmonst.data, AD_CORR))
	    is_acid = TRUE;
	else if (dmgtype(youmonst.data, AD_RUST))
	    is_acid = FALSE;
	else
	    return;

	if (youmonst.data == &mons[PM_WATER_ELEMENTAL]) {
	    if (rn2(2)) return;
            pline("%s is splashed by your water!", Monnam(mon));
	}

	vis = cansee(mon->mx, mon->my);

	if ((is_acid ? is_corrodeable(obj) : is_rustprone(obj)) &&
	    (is_acid ? obj->oeroded2 : obj->oeroded) < MAX_ERODE) {
		if (obj->greased || obj->oerodeproof || (obj->blessed && rn2(3))) {
		    if (vis)
			pline("Somehow, %s weapon is not affected.",
						s_suffix(mon_nam(mon)));
		    if (obj->greased && !rn2(2)) obj->greased = 0;
		} else {
		    if (vis)
			pline("%s %s%s!",
			        s_suffix(Monnam(mon)),
				aobjnam(obj, (is_acid ? "corrode" : "rust")),
			        (is_acid ? obj->oeroded2 : obj->oeroded)
				    ? " further" : "");
		    if (is_acid) obj->oeroded2++;
		    else obj->oeroded++;
		}
	}
}

#endif /* OVLB */
#ifdef OVL1

int
could_seduce(magr,mdef,mattk)
struct monst *magr, *mdef;
struct attack *mattk;
/* returns 0 if seduction impossible,
 *	   1 if fine,
 *	   2 if wrong gender for nymph */
{
	register struct permonst *pagr;
	boolean agrinvis, defperc;
	xchar genagr, gendef;

	if (is_animal(magr->data)) return (0);
	if(magr == &youmonst) {
		pagr = youmonst.data;
		agrinvis = (Invis != 0);
		genagr = poly_gender();
	} else {
		pagr = magr->data;
		agrinvis = magr->minvis;
		genagr = gender(magr);
	}
	if(mdef == &youmonst) {
		defperc = (See_invisible != 0);
		gendef = poly_gender();
	} else {
		defperc = sees_invis(mdef);
		gendef = gender(mdef);
	}

	if(agrinvis && !defperc
#ifdef SEDUCE
		&& mattk && mattk->adtyp != AD_SSEX
#endif
		)
		return 0;

	if(pagr->mlet != S_NYMPH
		&& ((pagr != &mons[PM_INCUBUS] && pagr != &mons[PM_SUCCUBUS])
#ifdef SEDUCE
		    || (mattk && mattk->adtyp != AD_SSEX)
#endif
		   ))
		return 0;
	
	if(genagr == 1 - gendef)
		return 1;
	else
		return (pagr->mlet == S_NYMPH) ? 2 : 0;
}

#endif /* OVL1 */
#ifdef OVLB

#ifdef SEDUCE
/* Returns 1 if monster teleported */
int
doseduce(mon)
register struct monst *mon;
{
	register struct obj *ring, *nring;
	boolean fem = (mon->data == &mons[PM_SUCCUBUS]); /* otherwise incubus */
	char qbuf[QBUFSZ];

	if (mon->mcan || mon->mspec_used) {
		pline("%s acts as though %s has got a %sheadache.",
		      Monnam(mon), mhe(mon),
		      mon->mcan ? "severe " : "");
		return 0;
	}

	if (multi < 0) {
		pline("%s seems dismayed at your lack of response.",
		      Monnam(mon));
		return 0;
	}

	if (Blind) pline("It caresses you...");
	else You_feel("very attracted to %s.", mon_nam(mon));

	for(ring = invent; ring; ring = nring) {
	    nring = ring->nobj;
	    if (ring->otyp != RIN_ADORNMENT) continue;
	    if (fem) {
		if (rn2(20) < ACURR(A_CHA)) {
		    Sprintf(qbuf, "\"That %s looks pretty.  May I have it?\"",
			safe_qbuf("",sizeof("\"That  looks pretty.  May I have it?\""),
			xname(ring), simple_typename(ring->otyp), "ring"));
		    makeknown(RIN_ADORNMENT);
		    if (yn(qbuf) == 'n') continue;
		} else pline("%s decides she'd like your %s, and takes it.",
			Blind ? "She" : Monnam(mon), xname(ring));
		makeknown(RIN_ADORNMENT);
		if (ring==uleft || ring==uright) Ring_gone(ring);
		if (ring==uwep) setuwep((struct obj *)0);
		if (ring==uswapwep) setuswapwep((struct obj *)0);
		if (ring==uquiver) setuqwep((struct obj *)0);
		freeinv(ring);
		(void) mpickobj(mon,ring);
	    } else {
		char buf[BUFSZ];

		if (uleft && uright && uleft->otyp == RIN_ADORNMENT
				&& uright->otyp==RIN_ADORNMENT)
			break;
		if (ring==uleft || ring==uright) continue;
		if (rn2(20) < ACURR(A_CHA)) {
		    Sprintf(qbuf,"\"That %s looks pretty.  Would you wear it for me?\"",
			safe_qbuf("",
			    sizeof("\"That  looks pretty.  Would you wear it for me?\""),
			    xname(ring), simple_typename(ring->otyp), "ring"));
		    makeknown(RIN_ADORNMENT);
		    if (yn(qbuf) == 'n') continue;
		} else {
		    pline("%s decides you'd look prettier wearing your %s,",
			Blind ? "He" : Monnam(mon), xname(ring));
		    pline("and puts it on your finger.");
		}
		makeknown(RIN_ADORNMENT);
		if (!uright) {
		    pline("%s puts %s on your right %s.",
			Blind ? "He" : Monnam(mon), the(xname(ring)), body_part(HAND));
		    setworn(ring, RIGHT_RING);
		} else if (!uleft) {
		    pline("%s puts %s on your left %s.",
			Blind ? "He" : Monnam(mon), the(xname(ring)), body_part(HAND));
		    setworn(ring, LEFT_RING);
		} else if (uright && uright->otyp != RIN_ADORNMENT) {
		    Strcpy(buf, xname(uright));
		    pline("%s replaces your %s with your %s.",
			Blind ? "He" : Monnam(mon), buf, xname(ring));
		    Ring_gone(uright);
		    setworn(ring, RIGHT_RING);
		} else if (uleft && uleft->otyp != RIN_ADORNMENT) {
		    Strcpy(buf, xname(uleft));
		    pline("%s replaces your %s with your %s.",
			Blind ? "He" : Monnam(mon), buf, xname(ring));
		    Ring_gone(uleft);
		    setworn(ring, LEFT_RING);
		} else impossible("ring replacement");
		Ring_on(ring);
		prinv((char *)0, ring, 0L);
	    }
	}

	if (!uarmc && !uarmf && !uarmg && !uarms && !uarmh
#ifdef TOURIST
								&& !uarmu
#endif
									)
		pline("%s murmurs sweet nothings into your ear.",
			Blind ? (fem ? "She" : "He") : Monnam(mon));
	else
		pline("%s murmurs in your ear, while helping you undress.",
			Blind ? (fem ? "She" : "He") : Monnam(mon));
	mayberem(uarmc, cloak_simple_name(uarmc));
	if(!uarmc)
		mayberem(uarm, "suit");
	mayberem(uarmf, "boots");
	if(!uwep || !welded(uwep))
		mayberem(uarmg, "gloves");
	mayberem(uarms, "shield");
	mayberem(uarmh, uarmh ? helmet_name(uarmh) : "helmet");
#ifdef TOURIST
	if(!uarmc && !uarm)
		mayberem(uarmu, "shirt");
#endif

	if (uarm || uarmc) {
		verbalize("You're such a %s; I wish...",
				flags.female ? "sweet lady" : "nice guy");
		if (!tele_restrict(mon)) (void) rloc(mon, FALSE);
		return 1;
	}
	if (u.ualign.type == A_CHAOTIC)
		adjalign(1);

	/* by this point you have discovered mon's identity, blind or not... */
	pline("Time stands still while you and %s lie in each other's arms...",
		noit_mon_nam(mon));
	if (rn2(35) > ACURR(A_CHA) + ACURR(A_INT)) {
		/* Don't bother with mspec_used here... it didn't get tired! */
		pline("%s seems to have enjoyed it more than you...",
			noit_Monnam(mon));
		switch (rn2(5)) {
			case 0: You_feel("drained of energy.");
				u.uen = 0;
				u.uenmax -= rnd(Half_physical_damage ? 5 : 10);
			        exercise(A_CON, FALSE);
				if (u.uenmax < 0) u.uenmax = 0;
				break;
			case 1: You("are down in the dumps.");
				(void) adjattrib(A_CON, -1, TRUE);
			        exercise(A_CON, FALSE);
				flags.botl = 1;
				break;
			case 2: Your("senses are dulled.");
				(void) adjattrib(A_WIS, -1, TRUE);
			        exercise(A_WIS, FALSE);
				flags.botl = 1;
				break;
			case 3:
				if (!resists_drli(&youmonst)) {
				    You_feel("out of shape.");
				    losexp("overexertion");
				} else {
				    You("have a curious feeling...");
				}
				break;
			case 4: {
				int tmp;
				You_feel("exhausted.");
			        exercise(A_STR, FALSE);
				tmp = rn1(10, 6);
				if(Half_physical_damage) tmp = (tmp+1) / 2;
				losehp(tmp, "exhaustion", KILLED_BY);
				break;
			}
		}
	} else {
		mon->mspec_used = rnd(100); /* monster is worn out */
		You("seem to have enjoyed it more than %s...",
		    noit_mon_nam(mon));
		switch (rn2(5)) {
		case 0: You_feel("raised to your full potential.");
			exercise(A_CON, TRUE);
			u.uen = (u.uenmax += rnd(5));
			break;
		case 1: You_feel("good enough to do it again.");
			(void) adjattrib(A_CON, 1, TRUE);
			exercise(A_CON, TRUE);
			flags.botl = 1;
			break;
		case 2: You("will always remember %s...", noit_mon_nam(mon));
			(void) adjattrib(A_WIS, 1, TRUE);
			exercise(A_WIS, TRUE);
			flags.botl = 1;
			break;
		case 3: pline("That was a very educational experience.");
			pluslvl(FALSE);
			exercise(A_WIS, TRUE);
			break;
		case 4: You_feel("restored to health!");
			u.uhp = u.uhpmax;
			if (Upolyd) u.mh = u.mhmax;
			exercise(A_STR, TRUE);
			flags.botl = 1;
			break;
		}
	}

	if (mon->mtame) /* don't charge */ ;
	else if (rn2(20) < ACURR(A_CHA)) {
		pline("%s demands that you pay %s, but you refuse...",
			noit_Monnam(mon),
			Blind ? (fem ? "her" : "him") : mhim(mon));
	} else if (u.umonnum == PM_LEPRECHAUN)
		pline("%s tries to take your money, but fails...",
				noit_Monnam(mon));
	else {
#ifndef GOLDOBJ
		long cost;

		if (u.ugold > (long)LARGEST_INT - 10L)
			cost = (long) rnd(LARGEST_INT) + 500L;
		else
			cost = (long) rnd((int)u.ugold + 10) + 500L;
		if (mon->mpeaceful) {
			cost /= 5L;
			if (!cost) cost = 1L;
		}
		if (cost > u.ugold) cost = u.ugold;
		if (!cost) verbalize("It's on the house!");
		else {
		    pline("%s takes %ld %s for services rendered!",
			    noit_Monnam(mon), cost, currency(cost));
		    u.ugold -= cost;
		    mon->mgold += cost;
		    flags.botl = 1;
		}
#else
		long cost;
                long umoney = money_cnt(invent);

		if (umoney > (long)LARGEST_INT - 10L)
			cost = (long) rnd(LARGEST_INT) + 500L;
		else
			cost = (long) rnd((int)umoney + 10) + 500L;
		if (mon->mpeaceful) {
			cost /= 5L;
			if (!cost) cost = 1L;
		}
		if (cost > umoney) cost = umoney;
		if (!cost) verbalize("It's on the house!");
		else { 
		    pline("%s takes %ld %s for services rendered!",
			    noit_Monnam(mon), cost, currency(cost));
                    money2mon(mon, cost);
		    flags.botl = 1;
		}
#endif
	}
	if (!rn2(25)) mon->mcan = 1; /* monster is worn out */
	if (!tele_restrict(mon)) (void) rloc(mon, FALSE);
	return 1;
}

STATIC_OVL void
mayberem(obj, str)
register struct obj *obj;
const char *str;
{
	char qbuf[QBUFSZ];

	if (!obj || !obj->owornmask) return;

	if (rn2(20) < ACURR(A_CHA)) {
		Sprintf(qbuf,"\"Shall I remove your %s, %s?\"",
			str,
			(!rn2(2) ? "lover" : !rn2(2) ? "dear" : "sweetheart"));
		if (yn(qbuf) == 'n') return;
	} else {
		char hairbuf[BUFSZ];

		Sprintf(hairbuf, "let me run my fingers through your %s",
			body_part(HAIR));
		verbalize("Take off your %s; %s.", str,
			(obj == uarm)  ? "let's get a little closer" :
			(obj == uarmc || obj == uarms) ? "it's in the way" :
			(obj == uarmf) ? "let me rub your feet" :
			(obj == uarmg) ? "they're too clumsy" :
#ifdef TOURIST
			(obj == uarmu) ? "let me massage you" :
#endif
			/* obj == uarmh */
			hairbuf);
	}
	remove_worn_item(obj, TRUE);
}
#endif  /* SEDUCE */

#endif /* OVLB */

#ifdef OVL1

STATIC_OVL int
passiveum(olduasmon,mtmp,mattk)
struct permonst *olduasmon;
register struct monst *mtmp;
register struct attack *mattk;
{
	int i, tmp;

	for (i = 0; ; i++) {
	    if (i >= NATTK) return 1;
	    if (olduasmon->mattk[i].aatyp == AT_NONE ||
	    		olduasmon->mattk[i].aatyp == AT_BOOM) break;
	}
	if (olduasmon->mattk[i].damn)
	    tmp = d((int)olduasmon->mattk[i].damn,
				    (int)olduasmon->mattk[i].damd);
	else if(olduasmon->mattk[i].damd)
	    tmp = d((int)olduasmon->mlevel+1, (int)olduasmon->mattk[i].damd);
	else
	    tmp = 0;

	/* These affect the enemy even if you were "killed" (rehumanized) */
	switch(olduasmon->mattk[i].adtyp) {
	    case AD_ACID:
		if (!rn2(2)) {
		    pline("%s is splashed by your acid!", Monnam(mtmp));
		    if (resists_acid(mtmp)) {
			pline("%s is not affected.", Monnam(mtmp));
			tmp = 0;
		    }
		} else tmp = 0;
		if (!rn2(30)) erode_armor(mtmp, TRUE);
		if (!rn2(6)) erode_obj(MON_WEP(mtmp), TRUE, TRUE);
		goto assess_dmg;
	    case AD_STON: /* cockatrice */
	    {
		long protector = attk_protection((int)mattk->aatyp),
		     wornitems = mtmp->misc_worn_check;

		/* wielded weapon gives same protection as gloves here */
		if (MON_WEP(mtmp) != 0) wornitems |= W_ARMG;

		if (!resists_ston(mtmp) && (protector == 0L ||
			(protector != ~0L &&
			    (wornitems & protector) != protector))) {
		    if (poly_when_stoned(mtmp->data)) {
			mon_to_stone(mtmp);
			return (1);
		    }
		    pline("%s turns to stone!", Monnam(mtmp));
		    stoned = 1;
		    xkilled(mtmp, 0, AD_STON);
		    if (mtmp->mhp > 0) return 1;
		    return 2;
		}
		return 1;
	    }
	    case AD_ENCH:	/* KMH -- remove enchantment (disenchanter) */
	    	if (otmp) {
	    	    (void) drain_item(otmp);
	    	    /* No message */
	    	}
	    	return (1);
	    default:
		break;
	}
	if (!Upolyd) return 1;

	/* These affect the enemy only if you are still a monster */
	if (rn2(3)) switch(youmonst.data->mattk[i].adtyp) {
	    case AD_PHYS:
	    	if (youmonst.data->mattk[i].aatyp == AT_BOOM) {
	    	    You("explode!");
		    u.mh = -1;
	    	    /* KMH, balance patch -- this is okay with unchanging */
		    killer_format = NO_KILLER_PREFIX;
	    	    rehumanize("exploded");
	    	    goto assess_dmg;
	    	}
	    	break;
	    case AD_PLYS: /* Floating eye */
		if (tmp > 127) tmp = 127;
		if (u.umonnum == PM_FLOATING_EYE) {
		    if (!rn2(4)) tmp = 127;
		    if (mtmp->mcansee && haseyes(mtmp->data) && rn2(3) &&
				(sees_invis(mtmp) || !Invis)) {
			if (Blind)
			    pline("As a blind %s, you cannot defend yourself.",
							youmonst.data->mname);
		        else {
			    if (mon_reflects(mtmp,
					    "Your gaze is reflected by %s %s."))
				return 1;
			    pline("%s is frozen by your gaze!", Monnam(mtmp));
			    mtmp->mcanmove = 0;
			    mtmp->mfrozen = tmp;
			    return 3;
			}
		    }
		} else { /* gelatinous cube */
		    pline("%s is frozen by you.", Monnam(mtmp));
		    mtmp->mcanmove = 0;
		    mtmp->mfrozen = tmp;
		    return 3;
		}
		return 1;
	    case AD_COLD: /* Brown mold or blue jelly */
		if (resists_cold(mtmp)) {
		    shieldeff(mtmp->mx, mtmp->my);
		    pline("%s is mildly chilly.", Monnam(mtmp));
		    golemeffects(mtmp, AD_COLD, tmp);
		    tmp = 0;
		    break;
		}
		pline("%s is suddenly very cold!", Monnam(mtmp));
		u.mh += tmp / 2;
		if (u.mhmax < u.mh) u.mhmax = u.mh;
		if (u.mhmax > ((youmonst.data->mlevel+1) * 8))
		    (void)split_mon(&youmonst, mtmp);
		break;
	    case AD_STUN: /* Yellow mold */
		if (!mtmp->mstun) {
		    mtmp->mstun = 1;
		    pline("%s %s.", Monnam(mtmp),
			  makeplural(stagger(mtmp->data, "stagger")));
		}
		tmp = 0;
		break;
	    case AD_FIRE: /* Red mold */
		if (resists_fire(mtmp)) {
		    shieldeff(mtmp->mx, mtmp->my);
		    pline("%s is mildly warm.", Monnam(mtmp));
		    golemeffects(mtmp, AD_FIRE, tmp);
		    tmp = 0;
		    break;
		}
		pline("%s is suddenly very hot!", Monnam(mtmp));
		break;
	    case AD_ELEC:
		if (resists_elec(mtmp)) {
		    shieldeff(mtmp->mx, mtmp->my);
		    pline("%s is slightly tingled.", Monnam(mtmp));
		    golemeffects(mtmp, AD_ELEC, tmp);
		    tmp = 0;
		    break;
		}
		pline("%s is jolted with your electricity!", Monnam(mtmp));
		break;
	    default: tmp = 0;
		break;
	}
	else tmp = 0;

    assess_dmg:
	if((mtmp->mhp -= tmp) <= 0) {
		pline("%s dies!", Monnam(mtmp));
		xkilled(mtmp,0,olduasmon->mattk[i].adtyp);
		if (mtmp->mhp > 0) return 1;
		return 2;
	}
	return 1;
}

#endif /* OVL1 */
#ifdef OVLB

#include "edog.h"
struct monst *
cloneu()
{
	register struct monst *mon;
	int mndx = monsndx(youmonst.data);

	if (u.mh <= 1) return(struct monst *)0;
	if (mvitals[mndx].mvflags & G_EXTINCT) return(struct monst *)0;
	mon = makemon(youmonst.data, u.ux, u.uy, NO_MINVENT|MM_EDOG);
	mon = christen_monst(mon, plname);
	initedog(mon);
	mon->m_lev = youmonst.data->mlevel;
	mon->mhpmax = u.mhmax;
	mon->mhp = u.mh / 2;
	u.mh -= mon->mhp;
	flags.botl = 1;
	return(mon);
}

#endif /* OVLB */

/*mhitu.c*/
