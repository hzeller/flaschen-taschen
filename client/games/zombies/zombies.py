#!/usr/bin/env python3

#Font: https://www.urbanfonts.com/fonts/Arcade.htm

#Built on 2018-12-15 by Richard Barnes (rbarnes.org)
#Started:         19:53
#Progress report: 21:30 - bullets kill zombies, zombies chase, title screen, run
#Stopped at:      22:10 - added ammo counters, life bars, increasing hoard generation speed, losing screen

import socket
from tkinter import *
import random
import time

WIDTH  = 45
HEIGHT = 35

title="""XXXXXXX                XX    XX
    XX                 XX    
   XX   XXXXXX XXX XX  XXXXX XX  XXXXX  XXXXX  
  XX    X    X XX X XX XX  X XX XX   XX X       
 XX     X    X XX X XX XX  X XX XXXXXXX XXXXX                 
XX      X    X XX X XX XX  X XX XX          X
XXXXXXX XXXXXX XX X XX XXXXX XX  XXXXX  XXXXX        

                  X   X   X
                  X    X X
                  XXX   X
                  X X   X
                  XXX   X

XXXXXX  XX        XX                       XX
XX   XX           XX                       XX
XX   XX XX  XXXXX XXXXXX  XXXXX  XX XX XXXXXX
XXXXXX  XX XX   X XX  XX      XX XXX   XX  XX
XX XX   XX XX     XX  XX  XXXXXX XX    XX  XX
XX  XX  XX XX   X XX  XX XX   XX XX    XX  XX
XX   XX XX  XXXXX XX  XX  XXXXXX XX    XXXXXX"""

lose="""XX   XX    XX                            XX      
XX   XX    XX                            XX      
XX   XX    XX      XXXXXX XXXXX  XXXXX   XX                             
XX   XX    XX      X    X X     XX   XX  XX                              
XX   XX    XX      X    X XXXXX XXXXXXX                                              
XX   XX    XX      X    X     X XX       XX                           
 XXXXX     XXXXXXX XXXXXX XXXXX  XXXXX   XX"""

def sign(x):
  if x<0:
    return -1
  elif x>0:
    return 1
  else:
    return 0

def wrap(val,maxval):
  if val<0:
    return maxval-1
  elif val==maxval:
    return 0
  else:
    return val

class Flaschen(object):
  '''A Framebuffer display interface that sends a frame via UDP.'''

  def __init__(self, host, port, width, height, layer=5, transparent=False):
    '''

    Args:
      host: The flaschen taschen server hostname or ip address.
      port: The flaschen taschen server port number.
      width: The width of the flaschen taschen display in pixels.
      height: The height of the flaschen taschen display in pixels.
      layer: The layer of the flaschen taschen display to write to.
      transparent: If true, black(0, 0, 0) will be transparent and show the layer below.
    '''
    self.width = width
    self.height = height
    self.layer = layer
    self.transparent = transparent
    self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    self._sock.connect((host, port))
    header = ''.join(["P6\n",
                      "%d %d\n" % (self.width, self.height),
                      "255\n"]).encode('utf-8')
    footer = ''.join(["0\n",
                      "0\n",
                      "%d\n" % self.layer]).encode('utf-8')
    self._data = bytearray(width * height * 3 + len(header) + len(footer))
    self._data[0:len(header)] = header
    self._data[-1 * len(footer):] = footer
    self._header_len = len(header)
    self._footer_len = len(footer)
    self._screen_len = len(self._data)-self._header_len-self._footer_len

  def clear(self):
    self._data[self._header_len+1:-self._footer_len] = ('0'*self._screen_len).encode('utf-8')

  def set(self, x, y, color):
    '''Set the pixel at the given coordinates to the specified color.

    Args:
      x: x offset of the pixel to set
      y: y offset of the piyel to set
      color: A 3 tuple of (r, g, b) color values, 0-255
    '''
    if x >= self.width or y >= self.height or x < 0 or y < 0:
      return
    if color == (0, 0, 0) and not self.transparent:
      color = (1, 1, 1)

    offset = (x + y * self.width) * 3 + self._header_len
    self._data[offset] = color[0]
    self._data[offset + 1] = color[1]
    self._data[offset + 2] = color[2]
  
  def send(self):
    '''Send the updated pixels to the display.'''
    self._sock.send(self._data)

class Character:
  def update(self, t, heros, npcs, occupied):
    pass
  def moveToward(self, x, y, occupied):
    newx = wrap(self.x + sign(x-self.x),WIDTH)
    newy = wrap(self.y + sign(y-self.y),HEIGHT)
    if occupied:
      if occupied[newy][newx]==0:
        occupied[self.y][self.x] = 0
        self.x = newx
        self.y = newy
        occupied[self.y][self.x] = 1
    else:
      self.x = newx
      self.y = newy      


class Hero(Character):
  def __init__(self):
    self.x       = WIDTH//2
    self.y       = HEIGHT//2
    self.color   = (0,255,0)
    self.type    = 'hero'
    self.dx      = 0
    self.dy      = 0
    self.last_dx = 0
    self.last_dy = 0
    self.speed   = 3
    self.ammo    = 40
    self.alive   = 40
  def update(self, t, heros, npcs, occupied):
    if t%self.speed==0:
      self.moveToward(self.x+self.dx, self.y+self.dy, None)


class Zombie(Character):
  def __init__(self):
    self.x     = 0
    self.y     = 0
    self.color = (255,0,0)
    self.type  = 'zombie'
    self.speed = 6
    self.alive = 1

    direc  = random.randint(0,4)
    if direc==0: #East
      self.y = random.randint(0,HEIGHT-1)
    elif direc==1: #North
      self.x = random.randint(0,WIDTH-1)
    elif direc==2: #West
      self.y = random.randint(0,HEIGHT-1)
      self.x = WIDTH-1
    elif direc==3: #South
      self.x = random.randint(0,WIDTH-1)
      self.y = HEIGHT-1

  def update(self, t, heros, npcs, occupied):
    if t%self.speed==0:
      self.moveToward(heros[0].x, heros[0].y, occupied)
    if self.x==heros[0].x and self.y==heros[0].y:
      heros[0].alive          -= 1
      self.alive               = 0
      occupied[self.y][self.x] = 0

class Bullet(Character):
  def __init__(self, x, y, dx, dy):
    self.x     = x
    self.y     = y
    self.color = (0,0,255)
    self.type  = 'bullet'
    self.speed = 1
    self.dx    = dx
    self.dy    = dy
    self.alive = 10

  def update(self, t, heros, npcs, occupied):
    self.alive = self.alive-1
    if t%self.speed==0:
      self.moveToward(self.x+self.dx, self.y+self.dy, None)
    if occupied[self.y][self.x]!=0:
      for n in npcs:
        if n.x==self.x and n.y==self.y:
          n.alive = 0
          occupied[self.y][self.x] = 0

class Ammo(Character):
  def __init__(self):
    self.x = random.randint(0,WIDTH-1)
    self.y = random.randint(0,HEIGHT-1)
    self.color = (0,0,255)
    self.type  = 'ammo'
    self.alive = 1
    self.colors = [(255,255,0), (255,0,255), (0,255,255)]

  def update(self, t, heros, npcs, occupied):
    h=heros[0]
    if h.x==self.x and h.y==self.y:
      h.ammo = min(h.ammo + 20,40)
      self.alive = 0
      return
    self.color = self.colors[t%3]

class Game:
  def __init__(self):
    self.screen = Flaschen(host='ft.noise', port=1337, width=WIDTH, height=HEIGHT, layer=5, transparent=False)
    self.npcs   = []
    self.heros  = [Hero()]
    self.main   = Tk()
    self.frame  = Frame(self.main, width=100, height=100)
    self.frame.pack()
    self.spawn_rate  = 1000
    self.update_rate = 25
    self.ammo_rate   = 5000
    self.t           = 0
    self.occupied    = [[0]*WIDTH for y in range(HEIGHT)]
    self.titleSequence()
    self.main.mainloop()

  def wordToScreen(self,word,color):
    for y,line in enumerate(word.split("\n")):
      for x,c in enumerate(line):
        if c=="X":
          self.screen.set(x,y,color)

  def titleSequence(self):
    colors = [(0,0,255),(0,255,0),(255,0,0)]
    count  = 0
    for x in range(10):
      count=count+1
      time.sleep(0.1)
      self.screen.clear()
      self.wordToScreen(title,colors[count%len(colors)])
      self.screen.send()
    self.screen.clear()
    self.screen.send()
    self.startGame()

  def loseSequence(self):
    self.main.destroy()
    self.wordToScreen(lose,(255,0,0))
    self.screen.send()

  def startGame(self):
    self.main.bind("<KeyPress>",     self.keyDown)
    # self.main.bind("<KeyRelease>",   self.keyUp)
    self.main.after(self.spawn_rate, self.spawnZombie)
    self.main.after(self.spawn_rate, self.spawnAmmo)
    self.main.after(self.update_rate, self.update)    

  def spawnZombie(self):
    self.npcs.append(Zombie())
    self.main.after(self.spawn_rate, self.spawnZombie)

  def spawnAmmo(self):
    self.npcs.append(Ammo())
    self.main.after(self.ammo_rate, self.spawnAmmo)

  def update(self):
    self.t = self.t+1

    for c in self.npcs:
      c.update(self.t, self.heros, self.npcs, self.occupied)
    for h in self.heros:
      h.update(self.t, self.heros, self.npcs, self.occupied)

    self.screen.clear()

    for c in self.npcs:
      c.x = wrap(c.x,WIDTH)
      c.y = wrap(c.y,WIDTH)
      self.screen.set(c.x,c.y,c.color)
    for h in self.heros:
      h.x  = wrap(h.x,WIDTH)
      h.y  = wrap(h.y,WIDTH)
      self.screen.set(h.x,h.y,h.color)

    for x in range(min(self.heros[0].alive,WIDTH)):
      self.screen.set(x,0,(255,255,0))
    for x in range(min(self.heros[0].ammo,WIDTH)):
      self.screen.set(x,1,(0,128,0))

    if self.heros[0].alive==0:
      self.loseSequence()

    self.screen.send()    

    self.spawn_rate = max(self.spawn_rate-50*sum([1 for x in self.npcs if x.type=='ammo' and x.alive==0]),200)

    self.npcs = [x for x in self.npcs if x.alive>0]

    self.main.after(self.update_rate, self.update)

  def fire(self, hero):
    if self.heros[hero].last_dx==0 and self.heros[hero].last_dy==0:
      return

    if self.heros[hero].ammo==0:
      return

    self.heros[hero].ammo -= 1

    self.npcs.append(Bullet(
      self.heros[hero].x, 
      self.heros[hero].y, 
      self.heros[hero].last_dx, 
      self.heros[hero].last_dy, 
    ))


  def keyDown(self, e):
    if e.keycode==113:
      self.heros[0].dx      = -1
      self.heros[0].dy      = 0
      self.heros[0].last_dx = -1
      self.heros[0].last_dy = 0
    elif e.keycode==114:
      self.heros[0].dx      = 1
      self.heros[0].dy      = 0
      self.heros[0].last_dx = 1
      self.heros[0].last_dy = 0
    elif e.keycode==111:
      self.heros[0].dx      = 0
      self.heros[0].dy      = -1
      self.heros[0].last_dy = -1
      self.heros[0].last_dx = 0
    elif e.keycode==116:
      self.heros[0].dx      = 0
      self.heros[0].dy      = 1
      self.heros[0].last_dy = 1
      self.heros[0].last_dx = 0
    elif e.char.lower()=='s':
      self.heros[0].dx = 0
      self.heros[0].dy = 0
    elif e.char.lower()=='q':
      sys.exit(0)
    elif e.char==' ':
      self.fire(0)
    else:
      print(e)

Game()
