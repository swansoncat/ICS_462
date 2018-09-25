/* bzflag
 * Copyright (c) 1993-2018 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
 *
 */

// interface header
#include "RobotPlayer.h"

// common implementation headers
#include "BZDBCache.h"

// local implementation headers
#include "World.h"
#include "Intersect.h"
#include "TargetingUtils.h"

//Added to use control panel object KNB
#include "playing.h"

std::vector<BzfRegion*>* RobotPlayer::obstacleList = NULL;

RobotPlayer::RobotPlayer(const PlayerId& _id, const char* _name,
				ServerLink* _server,
				const char* _motto = "") :
				LocalPlayer(_id, _name, _motto),
				target(NULL),
				pathIndex(0),
				timerForShot(0.0f),
				drivingForward(true)
{
  gettingSound = false;
  server       = _server;
}

// estimate a player's position at now+t, similar to dead reckoning
void RobotPlayer::projectPosition(const Player *targ,const float t,float &x,float &y,float &z) const
{
  double hisx=targ->getPosition()[0];
  double hisy=targ->getPosition()[1];
  double hisz=targ->getPosition()[2];
  double hisvx=targ->getVelocity()[0];
  double hisvy=targ->getVelocity()[1];
  double hisvz=targ->getVelocity()[2];
  double omega=fabs(targ->getAngularVelocity());
  double sx,sy;

  if ((targ->getStatus() & PlayerState::Falling) || fabs(omega) < 2*M_PI / 360 * 0.5)
  {
    sx=t*hisvx;
    sy=t*hisvy;
  }
  else
  {
    double hisspeed = hypotf(hisvx, hisvy);
    double alfa = omega * t;
    double r = hisspeed / fabs(omega);
    double dx = r * sin(alfa);
    double dy2 = r * (1 - cos(alfa));
    double beta = atan2(dy2, dx) * (targ->getAngularVelocity() > 0 ? 1 : -1);
    double gamma = atan2(hisvy, hisvx);
    double rho = gamma+beta;
    sx = hisspeed * t * cos(rho);
    sy = hisspeed * t * sin(rho);
  }
  x=(float)hisx+(float)sx;
  y=(float)hisy+(float)sy;
  z=(float)hisz+(float)hisvz*t;
  if (targ->getStatus() & PlayerState::Falling)
    z += 0.5f * BZDBCache::gravity * t * t;
  if (z < 0) z = 0;
}


// get coordinates to aim at when shooting a player; steps:
// 1. estimate how long it will take shot to hit target
// 2. calc position of target at that point of time
// 3. jump to 1., using projected position, loop until result is stable
void RobotPlayer::getProjectedPosition(const Player *targ, float *projpos) const
{
  double myx = getPosition()[0];
  double myy = getPosition()[1];
  double hisx = targ->getPosition()[0];
  double hisy = targ->getPosition()[1];
  double deltax = hisx - myx;
  double deltay = hisy - myy;
  double distance = hypotf(deltax,deltay) - BZDB.eval(StateDatabase::BZDB_MUZZLEFRONT) - BZDBCache::tankRadius;
  if (distance <= 0) distance = 0;
  double shotspeed = BZDB.eval(StateDatabase::BZDB_SHOTSPEED)*
    (getFlag() == Flags::Laser ? BZDB.eval(StateDatabase::BZDB_LASERADVEL) :
     getFlag() == Flags::RapidFire ? BZDB.eval(StateDatabase::BZDB_RFIREADVEL) :
     getFlag() == Flags::MachineGun ? BZDB.eval(StateDatabase::BZDB_MGUNADVEL) : 1) +
      hypotf(getVelocity()[0], getVelocity()[1]);

  double errdistance = 1.0;
  float tx, ty, tz;
  for (int tries=0 ; errdistance > 0.05 && tries < 4 ; tries++)
  {
    float t = (float)distance / (float)shotspeed;
    projectPosition(targ, t + 0.05f, tx, ty, tz); // add 50ms for lag
    double distance2 = hypotf(tx - myx, ty - myy);
    errdistance = fabs(distance2-distance) / (distance + ZERO_TOLERANCE);
    distance = distance2;
  }
  projpos[0] = tx; projpos[1] = ty; projpos[2] = tz;


  // projected pos in building -> use current pos
  if (World::getWorld()->inBuilding(projpos, 0.0f, BZDBCache::tankHeight)) {
    projpos[0] = targ->getPosition()[0];
    projpos[1] = targ->getPosition()[1];
    projpos[2] = targ->getPosition()[2];
  }  
}


void			RobotPlayer::doUpdate(float dt)
{
  LocalPlayer::doUpdate(dt);

  float tankRadius = BZDBCache::tankRadius;
  const float shotRange  = BZDB.eval(StateDatabase::BZDB_SHOTRANGE);
  const float shotRadius = BZDB.eval(StateDatabase::BZDB_SHOTRADIUS);

  // fire shot if any available
  timerForShot  -= dt;
  if (timerForShot < 0.0f)
    timerForShot = 0.0f;

  if (getFiringStatus() != Ready)
    return;

  bool	shoot   = false;
  const float azimuth = getAngle();
  // Allow shooting only if angle is near and timer has elapsed

  /*
  if ((!path.empty()) && timerForShot <= 0.0f) {
    float p1[3];
    getProjectedPosition(target, p1);
    const float* p2     = getPosition();
    float shootingAngle = atan2f(p1[1] - p2[1], p1[0] - p2[0]);
    if (shootingAngle < 0.0f)
      shootingAngle += (float)(2.0 * M_PI);
    float azimuthDiff   = shootingAngle - azimuth;
    if (azimuthDiff > M_PI)
      azimuthDiff -= (float)(2.0 * M_PI);
    else
      if (azimuthDiff < -M_PI)
	azimuthDiff += (float)(2.0 * M_PI);
    const float targetdistance = hypotf(p1[0] - p2[0], p1[1] - p2[1]) -
      BZDB.eval(StateDatabase::BZDB_MUZZLEFRONT) - tankRadius;
  */
  /*
    const float missby = fabs(azimuthDiff) *
      (targetdistance - BZDBCache::tankLength);
    // only shoot if we miss by less than half a tanklength and no building inbetween
    if (missby < 0.5f * BZDBCache::tankLength &&
	p1[2] < shotRadius) {
      float pos[3] = {getPosition()[0], getPosition()[1],
		      getPosition()[2] +  BZDB.eval(StateDatabase::BZDB_MUZZLEHEIGHT)};
      float dir[3] = {cosf(azimuth), sinf(azimuth), 0.0f};
      Ray tankRay(pos, dir);
      float maxdistance = targetdistance;
      if (!ShotStrategy::getFirstBuilding(tankRay, -0.5f, maxdistance))
      {
	shoot=true;
	// try to not aim at teammates
	for (int i=0; i <= World::getWorld()->getCurMaxPlayers(); i++)
	{
	  Player *p = 0;
	  if (i < World::getWorld()->getCurMaxPlayers())
	    p = World::getWorld()->getPlayer(i);
	  else
	    p = LocalPlayer::getMyTank();
	  if (!p || p->getId() == getId() || validTeamTarget(p) ||
	     !p->isAlive()) continue;
	  float relpos[3] = {getPosition()[0] - p->getPosition()[0],
			     getPosition()[1] - p->getPosition()[1],
			     getPosition()[2] - p->getPosition()[2]};
	  Ray ray(relpos, dir);
	  float impact = rayAtDistanceFromOrigin(ray, 5 * BZDBCache::tankRadius);
	  if (impact > 0 && impact < shotRange)
	  {
	    shoot=false;
	    timerForShot = 0.1f;
	    break;
	  }
	}
	if (shoot && fireShot()) {
	  // separate shot by 0.2 - 0.8 sec (experimental value)
	  timerForShot = float(bzfrand()) * 0.6f + 0.2f;
	}
      }
    }
  }*/
}

void			RobotPlayer::doUpdateMotion(float dt)
{
  if (isAlive()) {
    bool evading = false;
    // record previous position
    const float oldAzimuth = getAngle();
    const float* oldPosition = getPosition();
    float position[3];
    position[0] = oldPosition[0];
    position[1] = oldPosition[1];
    position[2] = oldPosition[2];
    float azimuth = oldAzimuth;
    float tankAngVel = BZDB.eval(StateDatabase::BZDB_TANKANGVEL);
    float tankSpeed = BZDBCache::tankSpeed;





    //start of Kalen modifications 1
    /* So what we are trying to do is first check if we are playin capture the flag, and then we create a float array to hold the position of the flag the robot is going to pursue.
    *  If we are in capture the flag game, the rest of the code runs. First we get the total flags in the game, and for each of them we check if it's a different TeamColor than our
    *  robot. If it is, then we get its position.
    *
    */
    bool teamGame = World::getWorld()->allowTeamFlags();
    float flagPos[3];
    bool flocking = true;
    float centerOfMass[3];
    float comDistance = 0;


    /*  This code is to move the center of mass calculation from the third if/else branch to the beginning of function.
     *     -current only calculates for LocalPlayer team. Need to implement for enemy tanks.
     *  This puts the center of mass and the distance to it into the variables 'centerOfMass' and 'comDistance'.
    */
    if (teamGame && flocking && LocalPlayer::getMyTank()->getTeam() == getTeam()) 
    {
	Player *p = 0;
	centerOfMass[0] = position[0] + LocalPlayer::getMyTank()->getPosition()[0];
	centerOfMass[1] = position[1] + LocalPlayer::getMyTank()->getPosition()[1];
	centerOfMass[2] = position[2] + LocalPlayer::getMyTank()->getPosition()[2];
	int numTeammates = 2;


	for (int t=0; t <= World::getWorld()->getCurMaxPlayers(); t++) 
	{
	    p = World::getWorld()->getPlayer(t);
	    if (p != NULL && p->getId() != getId() && p->getTeam() == getTeam())
	    {
		centerOfMass[0] = centerOfMass[0] + p->getPosition()[0];
		centerOfMass[1] = centerOfMass[1] + p->getPosition()[1];
		centerOfMass[2] = centerOfMass[2] + p->getPosition()[2];
		numTeammates++;
	    }   
	}
	centerOfMass[0] = (centerOfMass[0] / numTeammates);
	centerOfMass[1] = (centerOfMass[1] / numTeammates);
	//The below shouldn't really matter at this point in time, as the tank can't really just pick and choose what height it wants to be, but putting it there for consistency.
	centerOfMass[2] = (centerOfMass[2] / numTeammates);

	comDistance = hypotf(centerOfMass[0], centerOfMass[1]);  

	centerOfMass[0] = (centerOfMass[0] / numTeammates) - (2 * getRadius());
	centerOfMass[1] = (centerOfMass[1] / numTeammates) - (2 * getRadius());
	//The below shouldn't really matter at this point in time, as the tank can't really just pick and choose what height it wants to be, but putting it there for consistency.
	centerOfMass[2] = (centerOfMass[2] / numTeammates) - (2 * getRadius()); 

  
    }


    //Get robot tank to drop its own flag if it has it.
    if (teamGame && getFlag() != Flags::Null && (*(getFlag())).flagTeam == getTeam())
    {
	(*server).sendDropFlag(getId(), getPosition());
    }
    //Get robot tank to drop any white flags it accidentally picks up.
    if (teamGame && getFlag() != Flags::Null && (*(getFlag())).flagTeam == NoTeam)
    {
	(*server).sendDropFlag(getId(), getPosition());
    }



    /*  First if/else branch checks whether or not we are playing a teamGame and whether or not the robot is carrying a flag. Right now it is also checking whether or not
    *   it is on the same team as LocalPlayer (basically ally tanks will not seek enemy flag right now, only follow). The flocking variable is used to make the enemy tanks
    *   do nothing.
    *
    */
    if (teamGame && getFlag() == Flags::Null && LocalPlayer::getMyTank()->getTeam() != getTeam() /* && !flocking */ ) {
        int numberFlags = World::getWorld()->getMaxFlags();
	float flagDistance = 1000000; //arbitrarily large number.
	for (int f = 0; f < numberFlags; f++) {
	    Flag& flag = World::getWorld()->getFlag(f);
	    //Checks whether or not flag is it's own team or a non-team flag.
	    if ((*(flag.type)).flagTeam != getTeam() && (*(flag.type)).flagTeam != NoTeam) 
	    {
		//tries to find the lar
		if (hypotf(flag.position[0], flag.position[1]) < flagDistance)
		{
		    flagDistance = hypotf(flag.position[0], flag.position[1]);
		    flagPos[0] = flag.position[0];
		    flagPos[1] = flag.position[1];
		    flagPos[2] = flag.position[2];
		}
	    }

	}
	setFlagTarget(flagPos);
    }
    //else if statement checks if we are playing capture the flag and if is is true that the tank is holding a flag that belongs to an enemy team. This is to bring is 
    //back to its own base to score.
    else if (teamGame && getFlag() != Flags::Null && LocalPlayer::getMyTank()->getTeam() != getTeam() /* && !flocking */)
    {
	//TeamColor selfTeam = getTeam();
	const float* baseCoord = World::getWorld()->getBase(getTeam(),0);
	flagPos[0] = baseCoord[0];
	flagPos[1] = baseCoord[1];
	flagPos[2] = baseCoord[2];	  
	setFlagTarget(baseCoord);  
    }
    //This makes ally tanks follow my (LocalPlayer running bzflag) tank.
    else if (teamGame && flocking && LocalPlayer::getMyTank()->getTeam() == getTeam() && comDistance < (BZDBCache::worldSize / 8))
    {
	/*
	Player *p = 0;
	flagPos[0] = position[0] + LocalPlayer::getMyTank()->getPosition()[0];
	flagPos[1] = position[1] + LocalPlayer::getMyTank()->getPosition()[1];
	flagPos[2] = position[2] + LocalPlayer::getMyTank()->getPosition()[2];
	int numTeammates = 2;

	for (int t=0; t <= World::getWorld()->getCurMaxPlayers(); t++) 
	{
	    p = World::getWorld()->getPlayer(t);
	    if (p != NULL && p->getId() != getId() && p->getTeam() == getTeam())
	    {
		flagPos[0] = flagPos[0] + p->getPosition()[0];
		flagPos[1] = flagPos[1] + p->getPosition()[1];
		flagPos[2] = flagPos[2] + p->getPosition()[2];
		numTeammates++;
	    }   
	}
	*/

	centerOfMass[0] = centerOfMass[0] - (2 * getRadius());
	centerOfMass[1] = centerOfMass[1] - (2 * getRadius());
	//The below shouldn't really matter at this point in time, as the tank can't really just pick and choose what height it wants to be, but putting it there for consistency.
	centerOfMass[2] = centerOfMass[2] - (2 * getRadius()); 
	setFlagTarget(centerOfMass);


	flagPos[0] = centerOfMass[0];
	flagPos[1] = centerOfMass[1];
	flagPos[2] = centerOfMass[2];
    }
    else
    {
	///currently nothing here.
    }
    
    /*  This is the code example for printing to the console inside of the game.
    
    char buffer[128];
    sprintf (buffer, "getCurMaxPlayers() is  %d",
	World::getWorld()->getCurMaxPlayers());
    controlPanel->addMessage(buffer, 0);
  
    */

    //End of Kalen modifications







    // basically a clone of Roger's evasive code
    for (int t=0; t <= World::getWorld()->getCurMaxPlayers(); t++)
    {
      Player *p = 0;
      if (t < World::getWorld()->getCurMaxPlayers())
	p = World::getWorld()->getPlayer(t);
      else
	p = LocalPlayer::getMyTank();
      if (!p || p->getId() == getId())
	continue;
      const int maxShots = p->getMaxShots();
      for (int s = 0; s < maxShots; s++) {
	ShotPath* shot = p->getShot(s);
	if (!shot || shot->isExpired())
	  continue;
	// ignore invisible bullets completely for now (even when visible)
	if (shot->getFlag() == Flags::InvisibleBullet)
	  continue;

	const float* shotPos = shot->getPosition();
	if ((fabs(shotPos[2] - position[2]) > BZDBCache::tankHeight) && (shot->getFlag() != Flags::GuidedMissile))
	  continue;
	const float dist = TargetingUtils::getTargetDistance(position, shotPos);
	if (dist < 150.0f) {
	  const float *shotVel = shot->getVelocity();
	  float shotAngle = atan2f(shotVel[1], shotVel[0]);
	  float shotUnitVec[2] = {cosf(shotAngle), sinf(shotAngle)};

	  float trueVec[2] = {(position[0]-shotPos[0])/dist,(position[1]-shotPos[1])/dist};
	  float dotProd = trueVec[0]*shotUnitVec[0]+trueVec[1]*shotUnitVec[1];

	  if (dotProd > 0.97f) {
	    float rotation;
	    float rotation1 = (float)((shotAngle + M_PI/2.0) - azimuth);
	    if (rotation1 < -1.0f * M_PI) rotation1 += (float)(2.0 * M_PI);
	    if (rotation1 > 1.0f * M_PI) rotation1 -= (float)(2.0 * M_PI);

	    float rotation2 = (float)((shotAngle - M_PI/2.0) - azimuth);
	    if (rotation2 < -1.0f * M_PI) rotation2 += (float)(2.0 * M_PI);
	    if (rotation2 > 1.0f * M_PI) rotation2 -= (float)(2.0 * M_PI);

	    if (fabs(rotation1) < fabs(rotation2))
	      rotation = rotation1;
	    else
	      rotation = rotation2;
	    setDesiredSpeed(1.0f);
	    setDesiredAngVel(rotation);
	    evading = true;
	  }
	}
      }
    }

    // when we are not evading, follow the path

    /*
    if (!evading && dt > 0.0 && pathIndex < (int)path.size()) {
      float distance;
      float v[2];
      const float* endPoint = path[pathIndex].get();
      // find how long it will take to get to next path segment
      v[0] = endPoint[0] - position[0];
      v[1] = endPoint[1] - position[1];
      distance = hypotf(v[0], v[1]);
      float tankRadius = BZDBCache::tankRadius;
      // smooth path a little by turning early at corners, might get us stuck, though
      if (distance <= 2.5f * tankRadius)
	pathIndex++;
      float segmentAzimuth = atan2f(v[1], v[0]);
      float azimuthDiff = segmentAzimuth - azimuth;
      if (azimuthDiff > M_PI) azimuthDiff -= (float)(2.0 * M_PI);
      else if (azimuthDiff < -M_PI) azimuthDiff += (float)(2.0 * M_PI);
      if (fabs(azimuthDiff) > 0.01f) {
	// drive backward when target is behind, try to stick to last direction
	if (drivingForward)
	  drivingForward = fabs(azimuthDiff) < M_PI/2*0.9 ? true : false;
	else
	  drivingForward = fabs(azimuthDiff) < M_PI/2*0.3 ? true : false;
	setDesiredSpeed(drivingForward ? 1.0f : -1.0f);
	// set desired turn speed
	if (azimuthDiff >= dt * tankAngVel) {
	  setDesiredAngVel(1.0f);
	} else if (azimuthDiff <= -dt * tankAngVel) {
	  setDesiredAngVel(-1.0f);
	} else {
	  setDesiredAngVel(azimuthDiff / dt / tankAngVel);
	}
      } else {
	drivingForward = true;
	// tank doesn't turn while moving forward
	setDesiredAngVel(0.0f);
	// find how long it will take to get to next path segment
	if (distance <= dt * tankSpeed) {
	  pathIndex++;
	  // set desired speed
	  setDesiredSpeed(distance / dt / tankSpeed);
	} else {
	  setDesiredSpeed(1.0f);
	}
      }
    }
    */







    /* Beginning Kalen modifications 2.
    *  So this is a copy of the above code. We are going to try and modify it to set its target to where the flag is.
    *
    */    


    //(!evading && dt > 0.0 && pathIndex < (int)path.size())
    if (!evading && dt > 0.0 && pathIndex < (int)path.size()) {      
      float distance;
      float v[2];
      //const float* endPoint = path[pathIndex].get(); ----removed from original code
      // find how long it will take to get to next path segment
      //v[0] = endPoint[0] - position[0];
      //v[1] = endPoint[1] - position[1];
      v[0] = flagPos[0] - position[0];
      v[1] = flagPos[1] - position[1];
      distance = hypotf(v[0], v[1]);
      float tankRadius = BZDBCache::tankRadius;
      // smooth path a little by turning early at corners, might get us stuck, though
      if (distance <= 2.5f * tankRadius)
	  pathIndex++;

	  float segmentAzimuth = atan2f(v[1], v[0]);
	  float azimuthDiff = segmentAzimuth - azimuth;


	  if (azimuthDiff > M_PI) azimuthDiff -= (float)(2.0 * M_PI);
	  else if (azimuthDiff < -M_PI) azimuthDiff += (float)(2.0 * M_PI);
	  if (fabs(azimuthDiff) > 0.01f) {
	  // drive backward when target is behind, try to stick to last direction
	  if (drivingForward)
	    drivingForward = fabs(azimuthDiff) < M_PI/2*0.9 ? true : false;
	  else
	    drivingForward = fabs(azimuthDiff) < M_PI/2*0.3 ? true : false;
	  setDesiredSpeed(drivingForward ? 1.0f : -1.0f);
	  // set desired turn speed
	  if (azimuthDiff >= dt * tankAngVel) 
	  {
	    setDesiredAngVel(1.0f);
	  } 
	  else if (azimuthDiff <= -dt * tankAngVel) 
	  {
	    setDesiredAngVel(-1.0f);
	  } 
	  else 
	  {
	    setDesiredAngVel(azimuthDiff / dt / tankAngVel);
	  }
      }
      else {
	  drivingForward = true;
	  // tank doesn't turn while moving forward
	  setDesiredAngVel(0.0f);
	  // find how long it will take to get to next path segment
	  if (distance <= dt * tankSpeed) 
	  {
	    pathIndex++;
	    // set desired speed
	    setDesiredSpeed(distance / dt / tankSpeed);
	  } 
	  else
	  {
	    setDesiredSpeed(1.0f);
	  }
	}
      }

    }





  LocalPlayer::doUpdateMotion(dt);
}




void			RobotPlayer::setFlagTarget(const float* _target)
{
  static int mailbox = 0;
  
  path.clear();
  //target = _target;
  //if (!target) return;

  // work backwards (from target to me)
  //float proj[3];
  //getProjectedPosition(target, proj);
  const float *p1 = _target;
  const float* p2 = getPosition();
  float q1[2], q2[2];
  BzfRegion* headRegion = findRegion(p1, q1);
  BzfRegion* tailRegion = findRegion(p2, q2);
  if (!headRegion || !tailRegion) {
    // if can't reach target then forget it
    return;
  }

  mailbox++;
  headRegion->setPathStuff(0.0f, NULL, q1, mailbox);
  RegionPriorityQueue queue;
  queue.insert(headRegion, 0.0f);
  BzfRegion* next;
  while (!queue.isEmpty() && (next = queue.remove()) != tailRegion)
    findPath(queue, next, tailRegion, q2, mailbox);

  // get list of points to go through to reach the target
  next = tailRegion;
  do {
    p1 = next->getA();
    path.push_back(p1);
    next = next->getTarget();
  } while (next && next != headRegion);
  if (next || tailRegion == headRegion)
    path.push_back(q1);
  else
    path.clear();
  pathIndex = 0;
  
}



void RobotPlayer::getProjectedFlagPosition(const float *targ, float *projpos) const
{
  double myx = getPosition()[0];
  double myy = getPosition()[1];
  double hisx = targ[0];
  double hisy = targ[1];
  double deltax = hisx - myx;
  double deltay = hisy - myy;
  double distance = hypotf(deltax,deltay) - BZDB.eval(StateDatabase::BZDB_MUZZLEFRONT) - BZDBCache::tankRadius;
  if (distance <= 0) distance = 0;
  double shotspeed = BZDB.eval(StateDatabase::BZDB_SHOTSPEED)*
    (getFlag() == Flags::Laser ? BZDB.eval(StateDatabase::BZDB_LASERADVEL) :
     getFlag() == Flags::RapidFire ? BZDB.eval(StateDatabase::BZDB_RFIREADVEL) :
     getFlag() == Flags::MachineGun ? BZDB.eval(StateDatabase::BZDB_MGUNADVEL) : 1) +
      hypotf(getVelocity()[0], getVelocity()[1]);

  double errdistance = 1.0;
  float tx, ty, tz;
  for (int tries=0 ; errdistance > 0.05 && tries < 4 ; tries++)
  {
    float t = (float)distance / (float)shotspeed;
    //Need to fix this, just leaving here as I'm not sure whether I will outright delete this function or update so that it works.
    //projectPosition(targ, t + 0.05f, tx, ty, tz); // add 50ms for lag
    double distance2 = hypotf(tx - myx, ty - myy);
    errdistance = fabs(distance2-distance) / (distance + ZERO_TOLERANCE);
    distance = distance2;
  }
  projpos[0] = tx; projpos[1] = ty; projpos[2] = tz;


  // projected pos in building -> use current pos
  if (World::getWorld()->inBuilding(projpos, 0.0f, BZDBCache::tankHeight)) {
    projpos[0] = targ[0];
    projpos[1] = targ[1];
    projpos[2] = targ[2];
  }  
}



void			RobotPlayer::explodeTank()
{
  LocalPlayer::explodeTank();
  target = NULL;
  path.clear();
}

void			RobotPlayer::restart(const float* pos, float _azimuth)
{
  LocalPlayer::restart(pos, _azimuth);
  // no target
  path.clear();
  target = NULL;
  pathIndex = 0;

}

float			RobotPlayer::getTargetPriority(const
							Player* _target) const
{
  // don't target teammates or myself
  
  if (!this->validTeamTarget(_target))
    return 0.0f;

  // go after closest player
  // FIXME -- this is a pretty stupid heuristic
  const float worldSize = BZDBCache::worldSize;
  const float* p1 = getPosition();
  const float* p2 = _target->getPosition();

  float basePriority = 1.0f;
  // give bonus to non-paused player
  if (!_target->isPaused())
    basePriority += 2.0f;
  // give bonus to non-deadzone targets
  if (obstacleList) {
    float nearest[2];
    const BzfRegion* targetRegion = findRegion (p2, nearest);
    if (targetRegion && targetRegion->isInside(p2))
      basePriority += 1.0f;
  }
  return basePriority
    - 0.5f * hypotf(p2[0] - p1[0], p2[1] - p1[1]) / worldSize;
}

void		    RobotPlayer::setObstacleList(std::vector<BzfRegion*>*
						     _obstacleList)
{
  obstacleList = _obstacleList;
}

const Player*		RobotPlayer::getTarget() const
{
  return target;
}


/*So the issue we're having is we have no idea where this fucking robot's actions are initialzed...
*---it looks like the constructor initializes the RobotPlayer class using this setTarget() function
*
*/

void			RobotPlayer::setTarget(const Player* _target)
{
  static int mailbox = 0;
  
  path.clear();
  target = _target;
  if (!target) return;

  // work backwards (from target to me)
  float proj[3];
  getProjectedPosition(target, proj);
  const float *p1 = proj;
  const float* p2 = getPosition();
  float q1[2], q2[2];
  BzfRegion* headRegion = findRegion(p1, q1);
  BzfRegion* tailRegion = findRegion(p2, q2);
  if (!headRegion || !tailRegion) {
    // if can't reach target then forget it
    return;
  }

  mailbox++;
  headRegion->setPathStuff(0.0f, NULL, q1, mailbox);
  RegionPriorityQueue queue;
  queue.insert(headRegion, 0.0f);
  BzfRegion* next;
  while (!queue.isEmpty() && (next = queue.remove()) != tailRegion)
    findPath(queue, next, tailRegion, q2, mailbox);

  // get list of points to go through to reach the target
  next = tailRegion;
  do {
    p1 = next->getA();
    path.push_back(p1);
    next = next->getTarget();
  } while (next && next != headRegion);
  if (next || tailRegion == headRegion)
    path.push_back(q1);
  else
    path.clear();
  pathIndex = 0;
  
}

BzfRegion*		RobotPlayer::findRegion(const float p[2],
						float nearest[2]) const
{
  nearest[0] = p[0];
  nearest[1] = p[1];
  const int count = obstacleList->size();
  for (int o = 0; o < count; o++)
    if ((*obstacleList)[o]->isInside(p))
      return (*obstacleList)[o];

  // point is outside: find nearest region
  float      distance      = maxDistance;
  BzfRegion* nearestRegion = NULL;
  for (int i = 0; i < count; i++) {
    float currNearest[2];
    float currDistance = (*obstacleList)[i]->getDistance(p, currNearest);
    if (currDistance < distance) {
      nearestRegion = (*obstacleList)[i];
      distance = currDistance;
      nearest[0] = currNearest[0];
      nearest[1] = currNearest[1];
    }
  }
  return nearestRegion;
}

float			RobotPlayer::getRegionExitPoint(
				const float p1[2], const float p2[2],
				const float a[2], const float targetPoint[2],
				float mid[2], float& priority)
{
  float b[2];
  b[0] = targetPoint[0] - a[0];
  b[1] = targetPoint[1] - a[1];
  float d[2];
  d[0] = p2[0] - p1[0];
  d[1] = p2[1] - p1[1];

  float vect = d[0] * b[1] - d[1] * b[0];
  float t    = 0.0f;  // safe value
  if (fabs(vect) > ZERO_TOLERANCE) {
    // compute intersection along (p1,d) with (a,b)
    t = (a[0] * b[1] - a[1] * b[0] - p1[0] * b[1] + p1[1] * b[0]) / vect;
    if (t > 1.0f)
      t = 1.0f;
    else if (t < 0.0f)
      t = 0.0f;
  }

  mid[0] = p1[0] + t * d[0];
  mid[1] = p1[1] + t * d[1];

  const float distance = hypotf(a[0] - mid[0], a[1] - mid[1]);
  // priority is to minimize distance traveled and distance left to go
  priority = distance + hypotf(targetPoint[0] - mid[0], targetPoint[1] - mid[1]);
  // return distance traveled
  return distance;
}

void			RobotPlayer::findPath(RegionPriorityQueue& queue,
					BzfRegion* region,
					BzfRegion* targetRegion,
					const float targetPoint[2],
					int mailbox)
{
  const int numEdges = region->getNumSides();
  for (int i = 0; i < numEdges; i++) {
    BzfRegion* neighbor = region->getNeighbor(i);
    if (!neighbor) continue;

    const float* p1 = region->getCorner(i).get();
    const float* p2 = region->getCorner((i+1)%numEdges).get();
    float mid[2], priority;
    float total = getRegionExitPoint(p1, p2, region->getA(),
					targetPoint, mid, priority);
    priority += region->getDistance();
    if (neighbor == targetRegion)
      total += hypotf(targetPoint[0] - mid[0], targetPoint[1] - mid[1]);
    total += region->getDistance();
    if (neighbor->test(mailbox) || total < neighbor->getDistance()) {
      neighbor->setPathStuff(total, region, mid, mailbox);
      queue.insert(neighbor, priority);
    }
  }
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8
