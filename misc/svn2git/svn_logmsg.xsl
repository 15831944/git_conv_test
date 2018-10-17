<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:output method="text"/>
	<xsl:strip-space elements="*"/>
	<xsl:template match="/">
		<xsl:for-each select="log/logentry">
			<xsl:value-of select="msg"/>
		</xsl:for-each>
	</xsl:template>
</xsl:stylesheet>
