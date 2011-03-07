<?xml version="1.0" ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:output method="xml" indent="yes"/>

<xsl:template match="/">
<muse version="1.0">
	<xsl:apply-templates/>
</muse>
</xsl:template>

<xsl:template match="device">
			<MidiInstrument name="{@name}">
			  	<xsl:apply-templates/>
				<Controller name="Modulation" l="1" />
				<Controller name="reverb" l="3" />
				<Controller name="MainVolume" l="7" />
				<Controller name="Pan" l="10" />
				<Controller name="Expression" l="11" />
				<Controller name="Program" type="Program" init="0x0" />
			</MidiInstrument>
</xsl:template>

<xsl:template match="bank">
			<PatchGroup name="{@name}" >
			  	<xsl:apply-templates/>
			</PatchGroup>
</xsl:template>

<xsl:template match="program">
      			<Patch name="{@name}" hbank="{../@msb}" lbank="{../@lsb}" prog="{./@id}" />
</xsl:template>
</xsl:stylesheet>
