######################################################
# File: fred_make_map.R
# Authors: John Grefenstete with help from Andrew Topp
#
# usage: Rscript fred_make_map.R directory fips_code day
######################################################

######################################################
### Installing and loading required packages
######################################################
if (!require("maptools")) {
   install.packages("maptools", dependencies = TRUE)
   library(maptools)
}
if (!require("ggplot2")) {
   install.packages("ggplot2", dependencies = TRUE)
   library(ggplot2)
}
if (!require("rgdal")) {
   install.packages("rgdal", dependencies = TRUE)
   library(rgdal)
}
if (!require("rgeos")) {
   install.packages("rgeos", dependencies = TRUE)
   library(rgeos)
}
if (!require("maps")) {
   install.packages("maps", dependencies = TRUE)
   library(maps)
}
if (!require("sp")) {
   install.packages("sp", dependencies = TRUE)
   library(sp)
}

######################################################
# Reading command line args
######################################################
args <- commandArgs(trailingOnly = TRUE)
Dir <- args[1]
Day <- args[2]

######################################################
# Change working directory
######################################################
setwd(Dir)

######################################################
# Create map directory if needed
######################################################
system("mkdir -p MAPS")

######################################################
# Set name of output file
######################################################
# Outfile <- paste("MAPS/map-",Day,".pdf", sep="");
# pdf(Outfile)

# creates a 5 x 5 inch image
Outfile <- paste("MAPS/map-",Day,".png", sep="");
png(Outfile,    # create PNG for the heat map        
  width = 5*300,        # 5 x 300 pixels
  height = 5*300,
  res = 300,            # 300 pixels per inch
  pointsize = 10)        # smaller font size

######################################################
# Read in the data
######################################################
SimDay <- as.integer(Day)-1
Csvfile <- paste("households-",SimDay,".txt", sep="");
data <- read.table(file=Csvfile, header=TRUE, sep=",")

######################################################
# Get the title
######################################################
fileName <- 'TITLE'
Title <- readChar(fileName, file.info(fileName)$size - 1)

######################################################
# Get the FIPS code
######################################################
fileName <- 'FIPS'
FIPS <- readChar(fileName, file.info(fileName)$size - 1)

######################################################
# Get FRED_HOME
######################################################
FREDHOME <- Sys.getenv("FRED_HOME")
Cmd<-paste("echo ",FREDHOME," > ENV")
system(Cmd)

######################################################
# Shape file:
######################################################
TLfile <- paste("tl_2010_",FIPS, "_tract00", sep="")
shape.dir <- paste(FREDHOME, "/input_files/SHAPEFILES/2000/", TLfile, sep="")
Cmd<-paste("echo ",shape.dir,"/",TLfile," > SHAPEFILE",sep="")
system(Cmd)

block.shape <- readOGR(shape.dir, layer = TLfile)
# for 2010  shape files:
#block.df <- fortify(block.shape, region = "GEOID10")
# for 2000  shape files:
block.df <- fortify(block.shape, region = "CTIDFP00")

######################################################
# Check to make sure the FIPS codes line up
######################################################
# names(data)
# names(block.df)
# intersect(block.shape$GEOID10, data$GEO.id2)

######################################################
# Merge map and data
######################################################
# Map <- merge(block.df, data, by.x="id", by.y="GEO.id2")
Map <- block.df
Map <- Map[order(Map$order),]

######################################################
p <- ggplot(data=Map)
p <- p + geom_polygon(aes(x=long, y=lat, group=group),fill="white") + coord_equal()
p <- p + geom_path(aes(x=long, y=lat, group=group), color="gray60")
p <- p + geom_point(data=data, aes(long,lat),size=0.5,color="red")
p <- p + ggtitle(paste(Title,"\nFIPS = ",FIPS,"\nDay = ",Day,"\n",sep=""))
# p <- p + theme(plot.title = element_text(size = 14, face="bold"))
p <- p + theme(plot.title = element_text(size = 14))
p <- p + xlab("Longitude") + ylab("Latitude")
p


