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
FIPS <- args[2]
Day <- args[3]
Var <- args[4]

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
Csvfile <- paste("census_tracts-",SimDay,".txt", sep="");
data<-read.table(Csvfile, sep="\t", header=T)
data$GEO.id2<- as.double(data$GEO.id2) #format as numeric
data$Count <- as.numeric(data$Count)

######################################################
# Get FRED_HOME
######################################################
FREDHOME <- Sys.getenv("FRED_HOME")
Cmd<-paste("echo ",FREDHOME," > ENV")
system(Cmd)

######################################################
# Shape file:
######################################################
### 2000 shapefiles
### TLfile <- paste("tl_2010_",FIPS, "_tract00", sep="")
### shape.dir <- paste(FREDHOME, "/input_files/SHAPEFILES/2000/", TLfile, sep="")

### 2010 shapefiles
TLfile <- paste("tl_2010_",FIPS, "_tract10", sep="")
shape.dir <- paste(FREDHOME, "/input_files/SHAPEFILES/2010/", TLfile, sep="")

Cmd<-paste("echo ",shape.dir,"/",TLfile," > SHAPEFILE",sep="")
system(Cmd)

block.shape <- readOGR(shape.dir, layer = TLfile)

# for 2010  shape files:
block.df <- fortify(block.shape, region = "GEOID10")

# for 2000  shape files:
###block.df <- fortify(block.shape, region = "CTIDFP00")

######################################################
# Check to make sure the FIPS codes line up
######################################################
names(data)
names(block.df)
intersect(block.shape$GEOID10, data$GEO.id2)

######################################################
# Merge map and data
######################################################
Map <- merge(block.df, data, by.x="id", by.y="GEO.id2")
Map <- Map[order(Map$order),]

######################################################
# Get max value and increment for shading
######################################################
Maxvalue <- try(system("cat max", intern = TRUE))
Maxvalue <- as.integer(Maxvalue)
Increment <- as.integer((Maxvalue + 2.5) / 5)

# create a dataframe of locations to annotate
# places <- data.frame(lat = c(41.00), long = c(-80.00), name = c("Pittsburgh, PA"))

# creates a color palette from green to red
my_palette <- colorRampPalette(c("green", "yellow", "red"))(n = 299)

# (optional) defines the color breaks manually for a "skewed" color transition
col_breaks = c(seq(Maxvalue*0.5,Maxvalue,length=100),  # for red
  seq(Maxvalue*0.1,Maxvalue*0.5,length=100),              # for yellow
  seq(-1,Maxvalue*0.1,length=100))              # for green

######################################################
# ggplot mapping
Maxvalue <- 0.75 * Maxvalue
Midpoint <- Maxvalue / 2
b <- c(-1,Midpoint,Maxvalue)
Maxvalue = 250
######################################################
p <- ggplot(data=Map)
p <- p + geom_polygon(aes(x=long, y=lat, group=group, fill=Count)) + coord_equal()
p <- p + expand_limits(fill = seq(0, Maxvalue, by = 16))
p <- p + scale_fill_gradient(limit=c(0,Maxvalue), low = "white", high = "red", breaks=seq(0, Maxvalue, by=16))
# p <- p + scale_fill_gradientn(limits = c(0,Maxvalue), colours=c("blue","yellow","red"), breaks=b, labels=format(b))
# p <- p + scale_fill_gradient2(low = "green", mid = "yellow", high = "red", midpoint = Midpoint, space = "rgb", guide = "colourbar")
# p <- p + scale_fill_gradient(col = my_palette, breaks = col_breaks)
p <- p + geom_path(aes(x=long, y=lat, group=group), color='black')
p <- p + ggtitle(paste("FRED Influenza Model for FIPS = ",FIPS,"\nDay = ",Day,"\n",sep=""))
p <- p + theme(plot.title = element_text(size = 14, face="bold"))
p <- p + xlab("Longitude") + ylab("Latitude")
# p <- p + annotate("point", x=mydata$long, y=mydata$lat, color="blue")
# p <- p + annotate("point", x=places$long, y=places$lat, color="black")
# p <- p + annotate("text", x=places$long, y=places$lat, label=places$name, color="black", size=3)
p


