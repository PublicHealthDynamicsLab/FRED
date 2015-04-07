args <- commandArgs(TRUE)
mybottom <- as.numeric(args[1])
myleft <- as.numeric(args[2])
mytop <- as.numeric(args[3])
myright <- as.numeric(args[4])
filename <- args[5]
api_key <- args[6]
if (api_key == "none") {
 api_key = ''
}
lightness <- args[7]

library(ggmap)

# set user bounding box
location = c(left=myleft,bottom=mybottom,right=myright,top=mytop)

# convert user bounding box to center point for get_googlemap, 
# per method used in get_map
center <- c(
	lon = mean(location[c("left","right")]),
	lat = mean(location[c("bottom","top")])
)

#save user bbox just in case
user_bbox <- location

# starting point for bbox extension
extend <- 0.0
clipped <- TRUE

gsize <- 640

# loop until bbox is not clipped
while(clipped == TRUE) {
	# extend bbox by extend
	lons <- c(location['left'],location['right'])
	lats <- c(location['top'], location['bottom'])
	location <- make_bbox(lons, lats, f=extend)
	
	# calculate zoom from bounding box
	myzoom = calc_zoom(location)
	
	# get the google map from center point
	# uses get_googlemap because get_map doesn't allow for style attribute

	# used in FRED/measles
	my_style <- "lightness:0&feature:road|visibility:simplified&style=feature:poi|visibility:off&style=feature:administrative.locality|visibility:off&style=feature:administrative.neighborhood:off"

	if (lightness>0) {
		my_style <- "lightness:25&feature:road|visibility:simplified&style=feature:poi|visibility:off&style=feature:administrative.locality|visibility:off&style=feature:administrative.neighborhood:off"
	}

	map <- get_googlemap(center, key=api_key,zoom=myzoom, maptype="roadmap", style=my_style, size=c(gsize,gsize), scale=4)

	# retrieve bounding box from square google map
	gbottom <- attr(map, "bb")$ll.lat
	gtop <- attr(map, "bb")$ur.lat
	gleft <- attr(map, "bb")$ll.lon
	gright <- attr(map, "bb")$ur.lon
	
	# check retrieved bbox vs user bbox
	if(gbottom <= mybottom && gtop >= mytop && gleft <= myleft && gright >= myright) {
		clipped <- FALSE
	} else {
		# expand bbox extension 
		extend <- extend + 0.25
	}
}

gmap <- ggmap(map, extent="device")

# save map to png file
png(filename, width=gsize, height=gsize, units="px")
#png(filename)
gmap 
dev.off()

# print zoom level
cat("zoom:", myzoom, "\n")
# print bounding box
# cat("bbox:", gleft, gright, gtop, gbottom,"\n")
cat(gbottom, gleft, gtop, gright,"\n")
