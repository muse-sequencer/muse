#!/usr/bin/python
import sys
import os
import string

version = "0.6"
##########################################
##########################################
#
# MusE song format converter
#
#########################################
#########################################

# blocks to remove:
blocks = [
["<segmentsize>","</segmentsize>"],
["<rtcTicks>","</rtcTicks>"],
["<useAlsa>", "</useAlsa>"],
["<useJack>", "</useJack>"],
["<extendedMidi>", "</extendedMidi>"],
["<midiExportDivision>", "</midiExportDivision>"],
["<font", "</font"],
["<theme>", "</theme>"],
["_font", "/>"],
["<show_page_no>","</show_page_no>"],
["<show_measure_no>","</show_measure_no>"],
["<paper_width>","</paper_width>"],
["<paper_height>","</paper_height>"],
["<top_margin>","</top_margin>"],
["<bottom_margin>","</bottom_margin>"],
["<left_margin>","</left_margin>"],
["<right_margin>","</right_margin>"],
["<bars_page>","</bars_page>"],
["<print_scale>","</print_scale>"],
["<show_track_name>","</show_track_name>"],
["<activityColor","</activityColor>"],
["<activityMode>","</activityMode>"],
["<score>","</score>"],
["<image>","</image>"],
["<part_type>","</part_type>"],
["<show_events>","</show_events>"],
["<grid>","</grid>"],
["<printer type","</printer>"],
["<bigtime visible","</bigtime>"],
["<transport visible","</transport>"],
["<audioInput>","</audioInput>"],
["<audioOutput","</audioOutput>"],
["<AudioInRoute>","</AudioInRoute>"],
["<track type=", "<track type="],
["</track", "</track"],
["<staff","</staff"],
["<noteQuant>","</noteQuant"],
["<restQuant>","</restQuant"],
["<midiThru>","</midiThru"],
["<splitpoint>","</splitpoint"],
]

# To store array of audio groups
# input, group1,2,3,4 + master
AudioGroupTags = [
"<channels>","<connect>","<volume>","<pan>",
"<mute>","<solo>","<prefader>","<off>"]
adata=[]
synths=[]
clips=[]
midiports=[]

class MusEConvert:
		def __init__(self):
			self.insidePart = False
			self.insideWaveTrack=False
			self.currWaveTrackName=""
			
		# parse string and look for blocks to remove 
		def findAndSkipBlock(self, theFile, currLine):
				for line in blocks:
					if string.find(currLine , line[0]) > -1:
						#print "Skipping %s"%line[0]
						self.loopUntil(theFile, currLine, line[1])
						return True
				return False
		
		# when a block has been found, loop until the end-tag.
		def loopUntil(self, theFile, currLine, tagToFind):
				loopEnd = False
				if string.find(currLine , tagToFind) > -1:
						loopEnd = True
				while loopEnd == False:
						line = inFile.readline()
						if string.find(line, tagToFind) > -1:
								loopEnd = True

		def fixComma(self, theFile, currLine):
					newStr = string.replace(currLine , ",",".")
					return newStr
						
				
		def getSynth(self, theFile, currLine):
				line = theFile.readline() # <class>
				clas = self.getSimpleValue(line)
				line = theFile.readline() # <name>
				name = self.getSimpleValue(line)
				line = theFile.readline() # <port>
				port = self.getSimpleValue(line)
				line = theFile.readline() # <guiVisible>
				guiVisible = self.getSimpleValue(line)
				
				stateInfo = []
				line = theFile.readline()	
				notFoundStateEnd= True
				if string.find(line , "<midistate") > -1:
					notFoundStateEnd = True
					line = theFile.readline()	
					
					while notFoundStateEnd:
						if string.find(line , "<event") > -1:
							event = []
							event.append(line)
							notFoundEventEnd = True
							if string.find(line , "/>") > -1:
								notFoundEventEnd = False 
							line = theFile.readline()
							while notFoundEventEnd:
								if string.find(line , "</event") > -1:
									notFoundEventEnd = False
								event.append(line)
								line = theFile.readline()
							stateInfo.append(event)
						if string.find(line , "</midistate") > -1:
							notFoundStateEnd = False
				
				# got all events
				theFile.readline() # <audionode
				#        0    1    2    3          4         
				return ([clas,name,port,guiVisible,stateInfo])
					
		def getAudio(self,theFile,currLine, atype, name, idx):
				line = theFile.readline() # <channels>
				ch = self.getSimpleValue(line)
				
				conn = ""
				if atype == "audiomaster": # don't read any lines
					pass
				else:
					line = theFile.readline() # <connect>
					conn = self.getSimpleValue(line)
				
				line = theFile.readline() # <volume>
				vol = self.getSimpleValue(line)
				line = theFile.readline() # <pan>
				pan = self.getSimpleValue(line)
				
				line = theFile.readline() # <mute>
				mute = self.getSimpleValue(line)
				line = theFile.readline() # <solo>
				solo = self.getSimpleValue(line)
				
				line = theFile.readline() # <prefader>
				pre = self.getSimpleValue(line)
				
				line = theFile.readline() # <off>
				off = self.getSimpleValue(line)
				
				line = theFile.readline() # <plugin 1
				p1=[]
				p2=[]
				p3=[]
				p4=[]
				if string.find(line , "<plugin") > -1:
					notFoundPlugEnd = True
					while notFoundPlugEnd:
						p1.append(line)
						line = theFile.readline() # <plugin 1
						if string.find(line , "</plugin") > -1:
							p1.append(line)
							notFoundPlugEnd = False
				
					line = theFile.readline() # <plugin 2
					if string.find(line , "<plugin") > -1:
						notFoundPlugEnd = True
						while notFoundPlugEnd:
							p2.append(line)
							line = theFile.readline() # <plugin 2
							if string.find(line , "</plugin") > -1:
								p2.append(line)
								notFoundPlugEnd = False
						
						line = theFile.readline() # <plugin 3
						if string.find(line , "<plugin") > -1:
							notFoundPlugEnd = True
							while notFoundPlugEnd:
								p3.append(line)
								line = theFile.readline() # <plugin 3
								if string.find(line , "</plugin") > -1:
									p3.append(line)
									notFoundPlugEnd = False

							line = theFile.readline() # <plugin 4
							if string.find(line , "<plugin") > -1:
								notFoundPlugEnd = True
								while notFoundPlugEnd:
									p4.append(line)
									line = theFile.readline() # <plugin 4
									if string.find(line , "</plugin") > -1:
										p3.append(line)
										notFoundPlugEnd = False
					
				print "atype=", atype
				print "p1=", p1
				print "p2=", p2
				print "p3=", p3
				print "p4=", p4
				
				return([atype,name,idx,ch,conn,vol,pan,mute,solo,pre,off, p1,p2,p3,p4]) # add them together
					
		def checkAGroup(self, theFile, currLine):
				if string.find(currLine , "<audiogroup") > -1:
					print "AUDIOGROUP"
					theFile.readline() # <audiogroup>
					idx = self.getProperty(currLine, "idx")
					print "idx=",idx
					data = self.getAudio(theFile,currLine,"audiogroup", "Group %s"%(chr(int(idx)+65)),idx)
					adata.append(data)
					theFile.readline() # </audiogroup> 
					return True
				else:
					return False
		
		def checkSynth(self, theFile, currLine):
				if string.find(currLine , "<synth") > -1:
					print "SYNTH"
					synt = self.getSynth(theFile,currLine)
					data = self.getAudio(theFile,currLine,"synth", synt[1], 0)
					print "got synth, name=%s route=%s",synt[1], data[5]
					
					theFile.readline() # </synth>
					synths.append(synt)
					adata.append(data)

					return True
				else:
					return False
					
		def checkAMaster(self, theFile, currLine):
				if string.find(currLine , "<audiomaster") > -1:
					print "AUDIOMASTER"
					theFile.readline() # <audiomaster>
					data = self.getAudio(theFile,currLine,"audiomaster", "Master",-1)
					adata.append(data)
					theFile.readline() # </audiomaster> 
					return True
				else:
					return False
		
		def checkAInput(self, theFile, currLine):
				if string.find(currLine , "<audioinput") > -1:
					print "AUDIOINPUT"
					theFile.readline() # <audioinput>
					idx = self.getProperty(currLine, "idx")
					data = self.getAudio(theFile,currLine,"audioinput", "In 1",idx)
					adata.append(data)
					theFile.readline() # </audioinput> 
					return True
				else:
					return False
		
		# returns the property
		def getProperty(self, currLine, tag):
				prePtr = string.find(currLine , tag)
				if prePtr == -1:
					return -1
				firstPtr = string.find(currLine[prePtr:] , "\"")
				lastPtr = string.find(currLine[firstPtr+prePtr+1:] , "\"")
				return currLine[firstPtr+prePtr+1:firstPtr+prePtr+lastPtr+1]
				
		def checkMGroup(self, theFile, currLine):
				if string.find(currLine , "<midiport") > -1:
					print "MIDIPORT"
					idx = self.getProperty(currLine, "idx")
					line = theFile.readline() # <instrument>
					inst = self.getSimpleValue(line)
					line = theFile.readline() # <name>
					name = self.getSimpleValue(line)
					line = theFile.readline() # <record>
					rec = self.getSimpleValue(line)
					midiports.append([idx, inst, name,rec]) # add them together
					line = theFile.readline() # </midiport>
					return True
				else:
					return False
					
					
		# retrieve the value of a simple XML tag
		# format is important, must be:
		# <tag>value</tag>
		def getSimpleValue(self, line):
				firstPtr = string.find(line, ">") # find end of first tag
				lastPtr = string.find(line, "</")
				outStr = line[firstPtr+1:lastPtr]
				return outStr
		
		def checkClip(self, theFile, currLine):
				if string.find(currLine , "<clip>") > -1:
					if self.insidePart == False:
						print "CLIP"
						line = theFile.readline() # <file>
						fil = self.getSimpleValue(line)
						line = theFile.readline() # <name>
						nam = self.getSimpleValue(line)
						line = theFile.readline() # <name>
						tick = self.getSimpleValue(line)
						clips.append([nam, fil, tick]) # add them together
						line = theFile.readline() # <len>
						line = theFile.readline() # </clip>
					else:
						print "insertClip"
						clipname = self.getSimpleValue(currLine)
						for clip in clips:
							if clip[0] == clipname:
								outFile.write("            <frame>%s</frame>\n"%(clip[2]))
								outFile.write("            <file>%s</file>\n"%(clip[1]))
					return True
				else:
					return False
					


		def checkPart(self, theFile, currLine):
				if string.find(currLine , "<part>") > -1:
					print "PART"
					self.insidePart = True
				elif string.find(currLine , "</part>") > -1:
					print "/PART"
					self.insidePart = False
		
		def checkWaveTrack(self, theFile, currLine):
				if string.find(currLine , "</wavetrack>") > -1:
					print "/WAVETRACK"
					self.insideWaveTrack = False
					return False
				elif string.find(currLine , "<wavetrack>") > -1:
					print "WAVETRACK"
					self.insideWaveTrack = True
					return False
				
				if self.insideWaveTrack: # create dummy adata for the routing
					if string.find(currLine , "<connect>") > -1:
						con = self.getSimpleValue(currLine)
						
						#adata.append([atype,name,idx,ch,conn,vol,pan,mute,solo,pre,off, p1,p2,p3,p4])
						print ["wavetrack", self.currWaveTrackName,0,0,con,0,0,0,0,0,0,0,0,0,0]
						adata.append(["wavetrack", self.currWaveTrackName,0,0,con,0,0,0,0,0,0,0,0,0,0])
						
						return True
						
					elif string.find(currLine , "<audionode") > -1:
						print "AUDIONODE - in wave track"
						return True
					elif string.find(currLine , "</audionode") > -1:
						print "AUDIONODE - in wave track"
						return True
					elif string.find(currLine , "<volume>") > -1:
						vol = self.getSimpleValue(currLine)
						outFile.write("        <controller id=\"0\" cur=\"%s\">\n"%vol)
						outFile.write("          </controller>\n")
						return True
					elif string.find(currLine , "<pan>") > -1:
						pan = self.getSimpleValue(currLine)
						outFile.write("        <controller id=\"1\" cur=\"%s\">\n"%pan)
						outFile.write("          </controller>\n")
						return True
				return False

		def getWaveTrackName(self, theFile, currLine):
				if self.insideWaveTrack and not self.insidePart:
					if string.find(currLine , "<name>") > -1:
						print "WAVETRACK - NAME"
						self.currWaveTrackName = self.getSimpleValue(currLine)
						print "self.currWaveTrackName =", self.currWaveTrackName
				elif self.insideWaveTrack and self.insidePart:
					pass
				else:
					self.currWaveTrackName = ""
		
		def checkTriggerForAdd(self, theFile, currLine):
				if string.find(currLine , "<tempolist") > -1:
					# we're in business, add ALL stored info:
					# 1. AudioOutput
					# 2. AudioInput
					# 3. AudioGroup
					# 4. SynthI
					# 5. Routes
					#
					#               0     1    2   3  4    5   6   7    8    9   10  11 12 13 14
					# adata.append([atype,name,idx,ch,conn,vol,pan,mute,solo,pre,off,p1,p2,p3,p4])
					
					# 1. 
					for line in adata:
						if line[0] == "audiomaster":
							#print "amaster"
							#print line
							outFile.write("      <AudioOutput>\n")
							outFile.write("        <name>%s</name>\n"%line[1])
							outFile.write("        <record>0</record>\n")
							outFile.write("        <mute>%s</mute>\n"%line[7])
							outFile.write("        <solo>%s</solo>\n"%line[8])
							outFile.write("        <off>%s</off>\n"%line[10])
							outFile.write("        <channels>%s</channels>\n"%line[3])
							outFile.write("        <height>20</height>\n")
							outFile.write("        <locked>0</locked>\n")
							outFile.write("        <prefader>%s</prefader>\n"%line[9])
							outFile.write("        <automation>1</automation>\n")
							outFile.write("        <controller id=\"0\" cur=\"%s\">\n"%line[5])
							outFile.write("          </controller>\n")
							outFile.write("        <controller id=\"1\" cur=\"%s\">\n"%line[6])
							outFile.write("          </controller>\n")
							if line[11] !=[]:
								print "%s line[11] %s"%(line[1],line[11])
								for pl in line[11]:
									outFile.write(pl)
								if line[12] !=[]:
									print "%s line[12] %s"%(line[1],line[12])
									for pl in line[12]:
										outFile.write(pl)
									if line[13] !=[]:
										print "%s line[13] %s"%(line[1],line[13])
										for pl in line[13]:
											outFile.write(pl)
										if line[14] !=[]:
											print "%s line[14] %s"%(line[1],line[14])
											for pl in line[14]:
												outFile.write(pl)
							outFile.write("      </AudioOutput>\n")
					
					# 2. 
					for line in adata:
						if line[0] == "audioinput":
							outFile.write("      <AudioInput>\n")
							outFile.write("        <name>%s</name>\n"%line[1])
							outFile.write("        <record>0</record>\n")
							outFile.write("        <mute>%s</mute>\n"%line[7])
							outFile.write("        <solo>%s</solo>\n"%line[8])
							outFile.write("        <off>%s</off>\n"%line[10])
							outFile.write("        <channels>%s</channels>\n"%line[3])
							outFile.write("        <height>20</height>\n")
							outFile.write("        <locked>0</locked>\n")
							outFile.write("        <prefader>%s</prefader>\n"%line[9])
							outFile.write("        <automation>1</automation>\n")
							outFile.write("        <controller id=\"0\" cur=\"%s\">\n"%line[5])
							outFile.write("          </controller>\n")
							outFile.write("        <controller id=\"1\" cur=\"%s\">\n"%line[6])
							outFile.write("          </controller>\n")
							if line[11] !=[]:
								print "%s line[11] %s"%(line[1],line[11])
								for pl in line[11]:
									outFile.write(pl)
								if line[12] !=[]:
									print "%s line[12] %s"%(line[1],line[12])
									for pl in line[12]:
										outFile.write(pl)
									if line[13] !=[]:
										print "%s line[13] %s"%(line[1],line[13])
										for pl in line[13]:
											outFile.write(pl)
										if line[14] !=[]:
											print "%s line[14] %s"%(line[1],line[14])
											for pl in line[14]:
												outFile.write(pl)
							outFile.write("      </AudioInput>\n")
										                  
					# 3.                   
					for line in adata:     
						if line[0] == "audiogroup":
							outFile.write("      <AudioGroup>\n")
							outFile.write("        <name>%s</name>\n"%line[1])
							outFile.write("        <record>0</record>\n")
							outFile.write("        <mute>%s</mute>\n"%line[7])
							outFile.write("        <solo>%s</solo>\n"%line[8])
							outFile.write("        <off>%s</off>\n"%line[10])
							outFile.write("        <channels>%s</channels>\n"%line[3])
							outFile.write("        <height>20</height>\n")
							outFile.write("        <locked>0</locked>\n")
							outFile.write("        <prefader>%s</prefader>\n"%line[9])
							outFile.write("        <automation>1</automation>\n")
							outFile.write("        <controller id=\"0\" cur=\"%s\">\n"%line[5])
							outFile.write("          </controller>\n")
							outFile.write("        <controller id=\"1\" cur=\"%s\">\n"%line[6])
							outFile.write("          </controller>\n")
							if line[11] !=[]:
								print "%s line[11] %s"%(line[1],line[11])
								for pl in line[11]:
									outFile.write(pl)
								if line[12] !=[]:
									print "%s line[12] %s"%(line[1],line[12])
									for pl in line[12]:
										outFile.write(pl)
									if line[13] !=[]:
										print "%s line[13] %s"%(line[1],line[13])
										for pl in line[13]:
											outFile.write(pl)
										if line[14] !=[]:
											print "%s line[14] %s"%(line[1],line[14])
											for pl in line[14]:
												outFile.write(pl)
							outFile.write("      </AudioGroup>\n")

					# 4.                   
					for line in adata:     
						if line[0] == "synth":
							outFile.write("      <SynthI>\n")
							outFile.write("        <name>%s</name>\n"%line[1])
							outFile.write("        <record>0</record>\n")
							outFile.write("        <mute>%s</mute>\n"%line[7])
							outFile.write("        <solo>%s</solo>\n"%line[8])
							outFile.write("        <off>%s</off>\n"%line[10])
							outFile.write("        <channels>%s</channels>\n"%line[3])
							outFile.write("        <height>20</height>\n")
							outFile.write("        <locked>0</locked>\n")
							outFile.write("        <prefader>%s</prefader>\n"%line[9])
							outFile.write("        <automation>1</automation>\n")
							outFile.write("        <controller id=\"0\" cur=\"%s\">\n"%line[5])
							outFile.write("          </controller>\n")
							outFile.write("        <controller id=\"1\" cur=\"%s\">\n"%line[6])
							outFile.write("          </controller>\n")
							
# event data
							for synt in synths:
								if synt[1] == line[1]:
									# we found this synth, proceed
									outFile.write("        <class>%s</class>\n"%synt[0])
									outFile.write("        <port>%s</port>\n"%synt[2])
									outFile.write("        <guiVisible>%s</guiVisible>\n"%synt[3])
									outFile.write("        <midistate>\n")
									
									if synt[0] == "fluidsynth":
										# Woaaahh, special treatment, do I have something for you!!!!
										self.processFluid(synt)
									else:
										# Other synth
										for state in synt[4]:
											for evl in state:
												outFile.write(evl)
																
									outFile.write("        </midistate>\n")
							
							
							
					#return ([clas,name,port,guiVisible,stateInfo])
							
							outFile.write("      </SynthI>\n")
							
					#               0     1    2   3  4    5   6   7    8    9   10  11 12 13 14
					# adata.append([atype,name,idx,ch,conn,vol,pan,mute,solo,pre,off,p1,p2,p3,p4])
					
					# 5. - create all routes
					for line in adata:
						for line2 in adata:
								if line[4] == line2[1]:
									print ">route %s line[4]=%s  %s line2[1]=%s"%(line[1],line[4],line2[1],line2[1])
									outFile.write("      <Route>\n")
									outFile.write("        <srcNode>%s</srcNode>\n"%line[1])
									outFile.write("        <dstNode>%s</dstNode>\n"%line2[1])
									outFile.write("      </Route>\n")
								else:
									print "-route %s line[4]=%s  %s line2[1]=%s"%(line[1],line[4],line2[1],line2[1])
								
					outFile.write("    <Route>\n")
					outFile.write("      <srcNode>1:Master</srcNode>\n")
					outFile.write("      <dstNode>alsa_pcm:playback_1</dstNode>\n")
					outFile.write("    </Route>\n")
					outFile.write("    <Route>\n")
					outFile.write("      <srcNode>2:Master</srcNode>\n")
					outFile.write("      <dstNode>alsa_pcm:playback_2</dstNode>\n")
					outFile.write("    </Route>\n")
					
		def processFluid(self, fluid):
				# here we go
				print "Fluidsynth!!"
				#for state in fluid[4]:
				#	for evl in state:
				#		outFile.write(evl)
				
				# ok, let's convert the first event
				
				outdata="      "
				nbrOfFonts=0
				counter2f = 0
				externalConnects=[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
				externalConnects2f=[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
				for event in fluid[4]:
					hexstr = self.convertEventToHexStr(event)
					print hexstr
					counter1 = 0
					counter2 = 0
					if hexstr[0] == "28":
						externalConnects[nbrOfFonts] = hexstr[1]
						nbrOfFonts = nbrOfFonts+1
						for h in hexstr[2:-1]:
							outdata = outdata + "%s "%h
							counter1 == counter1 + 1
							counter2 == counter2 + 1
							if counter1 > 9:
								outdata = outdata + "\n       "
								counter1= 0
						
					elif hexstr[0] == "2f":
					  #                                            ext_id    preset    channel
						externalConnects2f[counter2f] = "%s %s %s "%(hexstr[1],hexstr[3], "00") #hexstr[2])
						counter2f=counter2f+1
						# translate
						# initStr = "f2 00 03 01 2f 00" # fluid v3 , one soundfont, lastdir= "/"
						
						
						
				outFile.write("    <event type=\"2\" datalen=\"2000\">\n") #%(len(hex)-3))
				outFile.write("    f2 00 03 0%X 2f 00\n"%nbrOfFonts)
				outFile.write(outdata)
				outFile.write("\n      ff ")
				for i in range(nbrOfFonts):
					outFile.write("%s "%(externalConnects[i]))
				outFile.write("\n      ")
				for ext in externalConnects2f:
					outFile.write(ext)
				outFile.write("\n      00 00 00 00 00 00 00 00\n")
				outFile.write("      00 00 00 00 ff ff ff ff 00 00 00 00 00 00\n")
					
					#outFile.write("      ff 00 00 81 00 7f 81 00 7f\n")
# 					outFile.write("      81 00 7f 81 00 7f 81 00 7f 81 00 7f 81 00 7f 81\n")
# 					outFile.write("      00 7f 81 00 7f 81 00 7f 81 00 7f 81 00 7f 81 00\n")
# 					outFile.write("      7f 81 00 7f 81 00 7f 81 00 00 00 00 00 00 00 00\n")
				outFile.write("    </event>\n")

				
						
		def convertEventToHexStr(self, event):
				# here we go
				newLine=[]
				for line in event[1:-1]:
					splitLine = string.split(line)
					nbr = len(splitLine)										
					#print nbr
					#print splitLine
					
					count = 0
					while count < nbr:
						newLine.append("%s%s"%(splitLine[count][1],splitLine[count+1][1]))
						count = count + 2
						
					#print newLine
					
				return newLine
							
				


def help():
		print "Utility to convert MusE 0.6 songs to 0.7 or newer song format."
		print "Usage: muse-convert <song file to convert>"
		print ""
		print "Please report any problems with this script to rj@spamatica.se"
		print "Author: Robert Jonsson, 2005, Copylefted under the GPL"
		
########################
# --- Main program --- #
########################
		
print "MusE Song converter %s"%version
print ""

if len(sys.argv) < 2:
	help()
	sys.exit("")
	
print "Processing file ", sys.argv[1]
print ""
# step 1 - remove malformed data
inFile = file(sys.argv[1])

outFile = file(sys.argv[1]+".tmp","w")
fileEnd = False

convert = MusEConvert()

#take care of first few lines:
outFile.write(inFile.readline()) # <?xml
line = inFile.readline() # <muse string
if string.find(line, "<muse version=\"1.0\">") == -1:
		print "Unable to confirm that this is a song file with the old format, aborting..."
		sys.exit("")
outFile.write("<muse version=\"2.0\" comment=\"converted by muse converter v%s\">\n"%version)


while fileEnd == False:
		line = inFile.readline()
		if line == "":
				fileEnd = True
		newLine = convert.fixComma(inFile,line)
		outFile.write(newLine)

inFile.close()
outFile.close()

#sys.exit("")
inFile = file(sys.argv[1]+".tmp","r")
outFile = file(sys.argv[1]+".converted","w")
fileEnd = False

while fileEnd == False:
		line = inFile.readline()
		if line == "":
				fileEnd = True
		
		convert.checkPart(inFile,line) # only state variables
		convert.getWaveTrackName(inFile,line) # only state variables
		
		convert.checkTriggerForAdd(inFile,line) # insert routes+stuff?
		
		if convert.findAndSkipBlock(inFile, line) == True:
				pass
		elif convert.checkAGroup(inFile,line):
				pass
		elif convert.checkAMaster(inFile,line):
				pass
		elif convert.checkSynth(inFile,line):
				pass
		elif convert.checkAInput(inFile,line):
				pass
		#elif convert.checkMGroup(inFile,line):
		#		pass
		elif convert.checkClip(inFile,line):
				pass
		elif convert.checkWaveTrack(inFile,line): # only state variables
				pass
		else:
				outFile.write(line)

print ""
print "Converted!"
print ""
