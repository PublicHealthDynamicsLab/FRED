#! /usr/bin/env python

# $Id: gaia.py,v 1.8 2013/05/05 01:06:01 stbrown Exp $ #

# Copyright 2009, Pittsburgh Supercomputing Center (PSC).  
# See the file 'COPYRIGHT.txt' for any restrictions.


import string,sys,os
import httplib
import math
import time

class GAIA:
    def __init__(self,plot_info_,conf_info_=None):
        assert(isinstance(plot_info_,PlotInfo))
        if conf_info_ is not None:
            assert(isinstance(conf_info_,ConfInfo))

        self.plotInfo = plot_info_
        if conf_info_ is None:
            self.confInfo = ConfInfo()
        else:
            self.confInfo = conf_info_

    def call(self):
        httpConn = httplib.HTTPConnection(self.confInfo.serverURL,self.confInfo.serverPort,True)
        if self.confInfo.debug:
            httpConn.set_debuglevel(2)
        httpConn.connect()

	time1 = time.time()
        xmlMessage = self.plotInfo.printXMLMessage()
	#with open("out.xml","wb") as f:
	#    f.write(xmlMessage)

	time2 = time.time()
	print "Took %g seconds to concatenate xml"%(time2-time1)
	time1 = time.time()
        headers = { Constants.MIME_CONTENT_TYPE: Constants.MIME_TEXT_XML,
                    Constants.MIME_CONTENT_LENGTH: '%d; %s="%s"'\
                    %(len(xmlMessage),Constants.MIME_CHARSET,Constants.MIME_ISO_8859_1)}
        httpConn.request("GET",self.confInfo.serverURL,"%s"%xmlMessage,headers)
	time2 = time.time()
	print "Sent request took %g seconds"%(time2-time1)
        response = httpConn.getresponse()
        ### determine the file we are returning
        responseFileType = response.getheader(Constants.MIME_CONTENT_TYPE)
        responseLength = response.getheader(Constants.MIME_CONTENT_LENGTH)
        if responseLength == 0:
            raise RuntimeError("GAIA Server returned an empty visuzalization")

        responseFileExt = ""
        if responseFileType in Constants.fileExtensions.keys():
            responseFileExt = Constants.fileExtensions[responseFileType]
        else:
            print 'WARNING: Unrecognized File Type returned from GAIA Server\n'\
                  'File Statistics:\n'\
                  '\tFile Type:%s\n'\
                  '\tFile Size:%d\n'\
                  '\tSaving to:%s'\
                  %(responseFileType,int(responseLength),self.plotInfo.output_filename+".gaia")
            responseFileExt = ".gaia"

        responseOutFile = self.plotInfo.output_filename + responseFileExt
        with open(responseOutFile,"wb") as f:
            f.write("%s"%response.read()) 

        if httpConn.sock:
            httpConn.sock.shutdown()
        httpConn.close()


class Constants:
    maximum_ABGR_value = int(255)
    minimum_ABGR_value = int(0)
    kNormalizedGeometry = float(1000.0)
    supportedOutputFormats = ['gif','png']
    supportedBundleFormats = ['tar','mpg','mov','mp4']
    supportedWrapperTypes = {'fips':5,'hasc':5,'usfips':5,'lonlat':6,'lonlat-label':7,'lonlat-path':-1,
                             'lonlat-poly':-1}
    GAIA_DEFAULT_STYLE = 0
    GAIA_DEFAULT_TIME_SEQ = -1
    WRAPPER_TYPE_VARIABLE = -1
    WRAPPER_RAW_ELEMENT_DELIMITER = ':'
    WRAPPER_VALUE_STYLE_DELIMITER = ':'
    GAIA_CLIENT_VERSION_NUMBER='0.1'
    WRAPPER_BUFFER_SIZE = 8192
    GAIA_INPUT_COMMENT_FLAG = "#"
    
    MIME_CONTENT_LENGTH    = "Content-Length"
    MIME_CONTENT_ENCODING  = "Content-Encoding"
    MIME_CONTENT_TYPE      = "Content-Type"
    MIME_TEXT_XML          = "text/xml"
    MIME_TEXT_PLAIN        = "text/plain"
    MIME_IMAGE_GIF         = "image/gif"
    MIME_IMAGE_PNG         = "image/png"
    MIME_VIDEO_MPG         = "video/mpeg"
    MIME_VIDEO_MOV         = "video/quicktime"
    MIME_VIDEO_MP4         = "video/mp4"
    MIME_VIDEO_OGG         = "video/ogg"
    MIME_APP_TAR           = "application/x-tar"
    MIME_APP_GZIP          = "application/x-gzip"
    MIME_APP_OCT_STREAM    = "application/octet-stream"
    MIME_BINARY            = MIME_APP_OCT_STREAM
    MIME_CHARSET           = "charset"
    MIME_ISO_8859_1        = "ISO-8859-1"
    MIME_CONTENT_TYPE_TEXT ="text/"

    fileExtensions = {MIME_IMAGE_GIF:'.gif',
                      MIME_IMAGE_PNG:'.png',
                      MIME_VIDEO_MPG:'.mpg',
		              MIME_VIDEO_MOV:'.mov',
		              MIME_VIDEO_MP4:'.mp4',
                      MIME_VIDEO_OGG:'.ogg',
                      MIME_APP_TAR:'.tgz',
                      MIME_APP_GZIP:'.gz'}

class aBGR:
    def __init__(self,alpha_=None, blue_=None, green_=None, red_=None, value=None):
        ### Alpha, blue, green, red values will override value
        if value is not None:
            if alpha_ is not None:
                raise RuntimeError("Cannot specify individual aBGR values and a string" \
                                   " value in the same instance of aBGR")            
            try:
                value_list = value.split(".")
                if len(value_list) == 4 :
                    alpha_ = int(value_list[0])
                    blue_  = int(value_list[1])
                    green_ = int(value_list[2])
                    red_   = int(value_list[3])
                else:
                    raise aBGRError
            except:
                raise RuntimeError("Format of String %s not suitable for an aBGR value"%value)      
        if alpha_ is not None:
            try:
                self.alpha = int(alpha_)
               
                if self.alpha < Constants.minimum_ABGR_value or self.alpha > Constants.maximum_ABGR_value:
                    raise aBGRError
            except:
                raise RuntimeError("Alpha value of %s invalid for an aBGR value"%alpha_) 
            try:
                self.blue = int(blue_)
                if self.blue < Constants.minimum_ABGR_value or self.blue > Constants.maximum_ABGR_value:
                    raise aBGRError
            except:
                raise RuntimeError("Blue value of %s invalid for an aBGR value"%blue_)
            try:
                self.green = int(green_)
                if self.green < Constants.minimum_ABGR_value or self.green > Constants.maximum_ABGR_value:
                    raise aBGRError
            except:
                raise RuntimeError("Green value of %s invalid for an aBGR value"%green_)
            try:
                self.red = int(red_)
                if self.red < Constants.minimum_ABGR_value or self.red > Constants.maximum_ABGR_value:
                    raise aBGRError
            except:
                raise RuntimeError("Red value of %s invalid for an aBGR value"%red_)
        else:
            raise RuntimeError("Arguments to aBGR instance do not make sense")
                
    def __str__(self):
        return "%d.%d.%d.%d"%(self.alpha,self.blue,self.green,self.red)

    def __repr__(self):
        return 'aBGR values:\n\talpha:%10d\n\tblue:%11d\n\tgreen:%10d\n\tred:%12d'%(self.alpha,
                                                                                    self.blue,
                                                                                    self.green,
                                                                                    self.red)
    def __sub__(self,otherColor):
        assert(isinstance(otherColor,aBGR))
        return (self.alpha - otherColor.alpha,
                self.blue - otherColor.blue,
                self.green - otherColor.green,
                self.red - otherColor.red)
    
class PlotInfo:
    def __init__(self,
                 input_filename_ = None,
                 output_filename_ = None,
                 styles_filenames_ = None,
                 wsdl_request_=False,
                 output_format_  ="png",
                 bundle_format_ = None,
                 start_color_ = None,
                 end_color_ = None,
                 start_radius_ = -1.0,
                 end_radius_ = -1.0,
                 num_gradients_ = 0,
                 max_resolution_ = Constants.kNormalizedGeometry,
                 project_image_ = False,
                 font_ = None,
                 font_size_ = 24.0,
                 legend_font_size_ = 16.0,
                 background_color_ = None,
                 fill_color_ = None,
                 stroke_width_ = 1.0,
                 title_ = None,
                 legend_ = None):

        self.input_filename = input_filename_
        if output_filename_ is None:
            self.output_filename = os.path.splitext(self.input_filename)[0]
        else:
            self.output_filename = output_filename_
        
        self.styles_filenames = styles_filenames_
        self.wsdl_request = False
        if wsdl_request_ is not None and wsdl_request_ is not False:
            self.wsdl_request = True
        self.output_format = output_format_
        self.bundle_format = bundle_format_
        self.start_color = None
        if start_color_ is not None:
            self.start_color = aBGR(value=start_color_)
            print "pI: " +str(self.start_color)
        self.end_color = None
        if end_color_ is not None:
            self.end_color = aBGR(value=end_color_)
        self.start_radius = float(start_radius_)
        self.end_radius = float(end_radius_)
        self.num_gradients = int(num_gradients_)
        self.max_resolution = float(max_resolution_)
        self.project_image = False
        if project_image_ is not None and project_image_ is not False:
            self.project_image = True
        self.font = font_
        self.font_size = float(font_size_)
        self.legend_font_size = float(legend_font_size_)
        self.background_color = None
        if background_color_ is not None:
            self.background_color = aBGR(value=background_color_)
        self.fill_color = None
        if fill_color_ is not None:
            self.fill_color = aBGR(value=fill_color_)
        self.stroke_width = float(stroke_width_)
        self.title = title_
        self.legend = legend_

        if self.input_filename is not None:
            self.wrappers = self.parseWrappers()
        else:
            self.wrappers = []
        self.styles = []
	if styles_filenames_ is not None:
	    for styleFilename in styles_filenames_:
		self.styles.append(self.parseStyles(styleFilename))
                   
    def parseWrappers(self):
        wrappers_tmp = []
        lonLatPathRecs = {}
        lonLatPolyRecs = {}
        with open(self.input_filename,"rb") as f:
            for line in f:
                elements = line.split()
                if elements[0][0]== Constants.GAIA_INPUT_COMMENT_FLAG:
                    continue
                elemType = elements[0].lower()
                
                element = None
                if elemType == "lonlat":
                    element = LonLat()
                    element.parseRec(line[len('lonlat '):])
                elif elemType == "usfips":
                    element = USFips()
                    element.parseRec(line[len('usfips '):])
                elif elemType == "hasc":
                    element = HASC()
                    element.parseRec(line[len('hasc '):])
                elif elemType == "lonlat-label":
                    element = LonLatLabel()
                    element.parseRec(line[len('lonlat-label '):])
                elif elemType == "lonlat-path":
                    elemID = int(elements[1])
                    if not lonLatPathRecs.has_key(elemID):
                        lonLatPathRecs[elemID] = []
                    elemStr = ""
                    for elem in elements[1:]:
                        elemStr += str(elem) + " "
                    lonLatPathRecs[elemID].append(elemStr)
                elif elemType == "lonlat-poly":
                    elemID = int(elements[1])
                    if not lonLatPolyRecs.has_key(elemID):
                        lonLatPolyRecs[elemID] = []
                    elemStr = ""
                    for elem in elements[1:]:
                        elemStr += str(elem) + " "
                    lonLatPolyRecs[elemID].append(elemStr)
                else:
                    raise RuntimeError("ERROR: Unsupported GAIA Element Type of %s in input"%elemType)

                if element is not None:
                    wrappers_tmp.append(element)

        ### Parse out the LonLatPaths
        if len(lonLatPathRecs) > 0:
            for lonLatID in lonLatPathRecs.keys():
                element = LonLatPath()
                element.parseRec(lonLatPathRecs[lonLatID])
                wrappers_tmp.append(element)
        ### Parse out the LonLatPolys
        if len(lonLatPolyRecs) > 0:
            for lonLatID in lonLatPolyRecs.keys():
                element = LonLatPoly()
                element.parseRec(lonLatPolyRecs[lonLatID])
                wrappers_tmp.append(element)
        return wrappers_tmp

    def parseStyles(self,styleFilename):
        styles_tmp = {}
        styles_tmp['elements'] = []
        styleID = 0
        legendSupport = 0
        with open(styleFilename,"rb") as f:
            lineList = f.readlines()
            for line in lineList:
                elements = line.split()
                if elements[0][0:2] == "id":
                    styleID = int(elements[0].split("=")[1])
                elif elements[0][0:14] == "legend-support":
                    legendSupport = int(elements[0].split("=")[1])
                else:
                    styles_tmp['elements'].append({'color':aBGR(value=elements[0]),
                                                   'radius':float(elements[1]),
                                                   'lower_bound':float(elements[2]),
                                                   'upper_bound':float(elements[3])})
        styles_tmp['styleID'] = int(styleID)
        styles_tmp['legend-support'] = int(legendSupport)
        return styles_tmp

    def printXMLMessage(self):
	xmlList = []
        xmlList.append('<?xml version="1.0" encoding="UTF-8" standalone="no" ?><gaia>')
        xmlList.append('<output-format>%s</output-format>'%(self.output_format))
        
        if self.bundle_format is not None:
            xmlList.append('<bundle-format>%s</bundle-format>'%(self.bundle_format))
            
        if self.num_gradients > 0:
            xmlList.append('<num-gradients>%d</num-gradients>'%(self.num_gradients))
            
        if self.max_resolution > 0:
            xmlList.append( '<max-resolution>%#g</max-resolution>'%(self.max_resolution))

        if self.start_color is not None:
            xmlList.append('<start-color>%s</start-color>'%(str(self.start_color)))

        if self.end_color is not None:
            xmlList.append('<end-color>%s</end-color>'%(str(self.end_color)))

        if self.start_radius > -1:
            xmlList.append('<start-radius>%g</start-radius>'%(self.start_radius))

        if self.end_radius > -1:
            xmlList.append('<end-radius>%g</end-radius>'%(self.end_radius))

        if self.font is not None:
            xmlList.append('<font-type>%s</font-type>'%(self.font))

        if self.font_size > -1:
            xmlList.append( '<font-size>%g</font-size>'%(self.font_size))

        if self.legend_font_size > -1:
            xmlList.append( '<legend-font-size>%g</legend-font-size>'%(self.legend_font_size))

        if self.background_color is not None:
            xmlList.append( '<background-color>%s</background-color>'%(self.background_color))

        if self.stroke_width > -1:
            xmlList.append( '<stroke-width>%g</stroke-width>'%(self.stroke_width))

        if self.project_image is True:
            xmlList.append( '<project-image>1</project-image>')
            
        if self.title is not None:
            xmlList.append( '<title>%s</title>'%self.title)

        if self.legend is not None:
            xmlList.append( '<legend-text>%s</legend-text>'%self.legend)


        ### Now write the styles
        if len(self.styles) > 0:
            for styleDict in self.styles:
                xmlList.append( '<style-range-list style-id="%d" legend-support="%d">'\
                            %(styleDict['styleID'],styleDict['legend-support']))
                for style in styleDict['elements']: 
                    xmlList.append('<color>%s</color><radius>%f</radius>'\
                                 '<lower-bound>%f</lower-bound>'\
                                 '<upper-bound>%f</upper-bound>'\
                                 %(str(style['color']),
                                   style['radius'],
                                   style['lower_bound'],
                                   style['upper_bound']))

                xmlList.append( '</style-range-list>')

        xmlList.append('<wrapper-raw>')
        wrapperCount = 0
        for wrapper in self.wrappers:
            elementStr = str(wrapper) + Constants.WRAPPER_RAW_ELEMENT_DELIMITER
            wrapperCount += len(elementStr)
            if wrapperCount > Constants.WRAPPER_BUFFER_SIZE:
                xmlList.append('</wrapper-raw><wrapper-raw>')
                wrapperCount = len(elementStr)
            xmlList.append(elementStr)
           
        xmlList.append('</wrapper-raw>')

        xmlList.append('</gaia>')
        xmlString = "".join(xmlList)
        return xmlString

class ConfInfo:
    def __init__(self,serverURL_="gaia.psc.edu",serverPort_="13500",logInfo_=None,debug_=False):
        self.serverURL = serverURL_
        self.serverPort = serverPort_
        self.logInfo = logInfo_
        self.debug = debug_

    def __str__(self):
        return "(%s,%d,%s)"%(self.serverURL,self.serverPort,self.logInfo)

    def __repr__(self):
        return "Configuration Information:\n\tServer:\t%s\n\tPort:\t%d\n\tLog:\t%s\n"\
              %(self.serverURL,self.serverPort,self.logInfo)


class Wrapper:
    def __init__(self,type_,
                 time_=Constants.GAIA_DEFAULT_TIME_SEQ,
                 styleID_=Constants.GAIA_DEFAULT_STYLE):
        self.type = type_.lower()
        self.time = time_
        self.styleID = styleID_
        
        if type_ not in Constants.supportedWrapperTypes.keys():
            raise RuntimeError("ERROR: Trying to create a Wrapper that is of an"\
                               "unsupported type %s in GAIA"%type_)

        self.info = {}

    def __str__(self):
        raise RuntimeError("Printing from the abstract Wrapper Class is not appropriate,"\
                           "please use a proper concrete class")

    def parseRec(self,rec):
        raise RuntimeError("Cannot parse record from abstract class Wrapper")


class LonLat(Wrapper):
    def __init__(self,longitude_=0.0,latitude_=0.0,value_=0.0,time_=Constants.GAIA_DEFAULT_TIME_SEQ,
                 styleID_=Constants.GAIA_DEFAULT_STYLE):
        Wrapper.__init__(self,"lonlat",time_,styleID_)
        self.latitude = float(latitude_)
        self.longitude = float(longitude_)
        self.value = float(value_)
    
    def parseRec(self,rec):
        ## split by space
        recSplit = rec.split()

        if len(recSplit) <3 or len(recSplit) > 4:
            raise RuntimeError("ERROR: lonlat element input has the incorrect number of fields")
        
        ## First look for a styleID parameter
        self.latitude = float(recSplit.pop(0))
        self.longitude = float(recSplit.pop(0))

        ### can have multiple values associated with styles
        foundFlag = False
        for record in recSplit:
            if Constants.WRAPPER_VALUE_STYLE_DELIMITER in record:
                if foundFlag:
                    raise RuntimeError("ERROR: Format of LonLat Wrapper has more than one Style parameter specified")
                foundFlag = True
                styleSplit = record.split(Constants.WRAPPER_VALUE_STYLE_DELIMITER)
                self.styleID = int(styleSplit[1])
                ### eliminate this from the record
                recSplit[recSplit.index(record)] = str(styleSplit[0])
        self.value = float(recSplit.pop(0))

        ### is there a time sequence value
        
        if len(recSplit) > 0:
            self.time = float(recSplit.pop(0))

    def __str__(self):
        return 'lonlat %f %f %g %d %g'%(self.longitude,self.latitude,\
                                        self.value,self.styleID,self.time)
        
class USFips(Wrapper):
    def __init__(self,fipsString_=None,value_=0.0,time_=Constants.GAIA_DEFAULT_TIME_SEQ,
                 styleID_=Constants.GAIA_DEFAULT_STYLE):
        Wrapper.__init__(self,"usfips",time_,styleID_)
        self.fipsString = fipsString_
        self.value = float(value_)

    def parseRec(self,rec):
        ## split by space
        recSplit = rec.split()

        if len(recSplit) < 2 or len(recSplit) > 3:
            raise RuntimeError("ERROR: usfips element input has the incorrect number of fields")
        
        self.fipsString = recSplit.pop(0)

        foundFlag = False
        for record in recSplit:
            if Constants.WRAPPER_VALUE_STYLE_DELIMITER in record:
                if foundFlag:
                    raise RuntimeError("ERROR: Format of LonLat Wrapper has more than one Style parameter specified")
                foundFlag = True
                styleSplit = record.split(Constants.WRAPPER_VALUE_STYLE_DELIMITER)
                self.styleID = int(styleSplit[1])
                ### eliminate this from the record
                recSplit[recSplit.index(record)] = str(styleSplit[0])
        self.value = float(recSplit.pop(0))

         ### is there a time sequence value
        if len(recSplit) > 0:
            self.time = float(recSplit.pop(0))

    def __str__(self):
        return "usfips %s %g %d %g"%(self.fipsString,self.value,self.styleID,self.time)

class HASC(Wrapper):
    def __init__(self,fipsString_=None,value_=0.0,time_=Constants.GAIA_DEFAULT_TIME_SEQ,
                 styleID_=Constants.GAIA_DEFAULT_STYLE):
        Wrapper.__init__(self,"hasc",time_,styleID_)
        self.fipsString = fipsString_
        self.value = float(value_)

    def parseRec(self,rec):
        ## split by space
        recSplit = rec.split()

        if len(recSplit) < 2 or len(recSplit) > 3:
            raise RuntimeError("ERROR: hasc element input has the incorrect number of fields")
        
        self.fipsString = recSplit.pop(0)

        foundFlag = False
        for record in recSplit:
            if Constants.WRAPPER_VALUE_STYLE_DELIMITER in record:
                if foundFlag:
                    raise RuntimeError("ERROR: Format of LonLat Wrapper has more than one Style parameter specified")
                foundFlag = True
                styleSplit = record.split(Constants.WRAPPER_VALUE_STYLE_DELIMITER)
                self.styleID = int(styleSplit[1])
                ### eliminate this from the record
                recSplit[recSplit.index(record)] = str(styleSplit[0])
        self.value = float(recSplit.pop(0))
        
        ### is there a time sequence value
        if len(recSplit) > 0:
            self.time = float(recSplit.pop(0))

    def __str__(self):
            return "hasc %s %g %d %g"%(self.fipsString,self.value,self.styleID,self.time)
        
class LonLatLabel(Wrapper):
    def __init__(self,longitude_=0.0,latitude_=0.0,label_="Label",value_=0.0,time_=Constants.GAIA_DEFAULT_TIME_SEQ,
                 styleID_=Constants.GAIA_DEFAULT_STYLE):
        Wrapper.__init__(self,"lonlat-label",time_,styleID_)
        self.longitude = float(longitude_)
        self.latitude = float(latitude_)
        self.value = float(value_)
        self.label = str(label_)

    def parseRec(self,rec):
        
        ### first we need to parse out the label between quotes
        if rec.find('"') == -1:
            raise RuntimeError("ERROR: lonlat-label element has no label in it")
        start = rec.index('"') + len('"')
        end = rec.index('"',start)
        self.label = rec[start:end]

        ### eliminate the label from the record and move on
        rec = rec[end+1:]

        ## split by space
        recSplit = rec.split()
        
        if len(recSplit) < 3 or len(recSplit) > 4:
            raise RuntimeError("ERROR: lonlat-label element input has the incorrect number of fields")
        
        self.latitude = float(recSplit.pop(0))
        self.longitude = float(recSplit.pop(0))

        foundFlag = False
        for record in recSplit:
            if Constants.WRAPPER_VALUE_STYLE_DELIMITER in record:
                if foundFlag:
                    raise RuntimeError("ERROR: Format of LonLat Wrapper has more than one Style parameter specified")
                foundFlag = True
                styleSplit = record.split(Constants.WRAPPER_VALUE_STYLE_DELIMITER)
                self.styleID = int(styleSplit[1])
                ### eliminate this from the record
                recSplit[recSplit.index(record)] = str(styleSplit[0])
        self.value = float(recSplit.pop(0))

        if len(recSplit) > 0:
            self.time = float(recSplit.pop(0))

    def __str__(self):
        return 'lonlat-label %f %f "%s" %g %d %g'%(self.longitude,self.latitude,self.label,\
                                                   self.value,self.styleID,self.time)
        

class LonLatPath(Wrapper):
    def __init__(self,coordinates_=None,pathID_=0,value_=0.0,time_=Constants.GAIA_DEFAULT_TIME_SEQ,
                 styleID_=Constants.GAIA_DEFAULT_STYLE):
        Wrapper.__init__(self,"lonlat-path",time_,styleID_)
        if coordinates_ is None:
            self.coordinates = []
        else:
            self.coordinates = coordinates_
        self.value = float(value_)
        self.pathID = int(pathID_)

### This is a special element, as it needs to be defined on more than one record
                 
    def parseRec(self,recs):
        assert(isinstance(recs,list))
        if len(recs) < 2:
            raise RuntimeError("ERROR: Must Define more than one lonlat point for a LonLatPath")

        second_pass = False
        for rec in recs:
            ## split by space
            recSplit = rec.split()

            pathID = int(recSplit.pop(0))
            latitude = float(recSplit.pop(0))
            longitude = float(recSplit.pop(0))
            styleID = Constants.GAIA_DEFAULT_STYLE
            time = Constants.GAIA_DEFAULT_TIME_SEQ
            
            foundFlag = False
            for record in recSplit:
                if Constants.WRAPPER_VALUE_STYLE_DELIMITER in record:
                    if foundFlag:
                        raise RuntimeError("ERROR: Format of LonLat Wrapper has more than one Style parameter specified")
                    foundFlag = True
                    styleSplit = record.split(Constants.WRAPPER_VALUE_STYLE_DELIMITER)
                    styleID = int(styleSplit[1])
                    ### eliminate this from the record
                    recSplit[recSplit.index(record)] = str(styleSplit[0])
            value = float(recSplit.pop(0))

            if len(recSplit) > 0:
                time = float(recSplit.pop(0))

            if second_pass:
                # If this is the second entry or greater... just error check
                if self.pathID != pathID or self.value != value or self.time != time:
                    raise RuntimeError("ERROR: Inconsistent Records in LonLatPath records")
            else:
                # If this is the first entry, set the values
                self.pathID = pathID
                self.value = value
                self.time = time
                self.styleID = styleID

            self.coordinates.append((longitude,latitude))
                    

    def __str__(self):
        returnString = "lonlat-path %d "%(self.pathID)
        for coords in self.coordinates:
            returnString += "%f %f, "%(coords[0],coords[1])

        return returnString[:-2] + " %g %d %g"%(self.value,self.styleID,self.time)
        
class LonLatPoly(Wrapper):
    def __init__(self,coordinates_=None,polyID_=0,value_=0.0,time_=Constants.GAIA_DEFAULT_TIME_SEQ,
                 styleID_=Constants.GAIA_DEFAULT_STYLE):
        Wrapper.__init__(self,"lonlat-path",time_,styleID_)
        if coordinates_ is None:
            self.coordinates = []
        else:
            self.coordinates = coordinates_
        self.value = float(value_)
        self.polyID = int(polyID_)

### This is a special element, as it needs to be defined on more than one record
                 
    def parseRec(self,recs):
        assert(isinstance(recs,list))
        if len(recs) < 2:
            raise RuntimeError("ERROR: Must Define more than one lonlat point for a LonLatPoly")

        second_pass = False
        for rec in recs:
            ## split by space
            recSplit = rec.split()

            polyID = int(recSplit.pop(0))
            latitude = float(recSplit.pop(0))
            longitude = float(recSplit.pop(0))
            styleID = Constants.GAIA_DEFAULT_STYLE
            time = Constants.GAIA_DEFAULT_TIME_SEQ
            
            foundFlag = False
            for record in recSplit:
                if Constants.WRAPPER_VALUE_STYLE_DELIMITER in record:
                    if foundFlag:
                        raise RuntimeError("ERROR: Format of LonLat Wrapper has more than one Style parameter specified")
                    foundFlag = True
                    styleSplit = record.split(Constants.WRAPPER_VALUE_STYLE_DELIMITER)
                    styleID = int(styleSplit[1])
                    ### eliminate this from the record
                    recSplit[recSplit.index(record)] = str(styleSplit[0])
            value = float(recSplit.pop(0))

            if len(recSplit) > 0:
                time = float(recSplit.pop(0))

            if second_pass:
                # If this is the second entry or greater... just error check
                if self.polyID != polyID or self.value != value or self.time != time:
                    raise RuntimeError("ERROR: Inconsistent Records in LonLatPoly records")
            else:
                # If this is the first entry, set the values
                self.polyID = polyID
                self.value = value
                self.time = time
                self.styleID = styleID

            self.coordinates.append((longitude,latitude))
                    

    def __str__(self):
        returnString = "lonlat-poly %d "%(self.polyID)
        for coords in self.coordinates:
            returnString += "%f %f, "%(coords[0],coords[1])

        return returnString[:-2] + " %g %d %g"%(self.value,self.styleID,self.time)
        
                
### Static Utility functions

def compressList(chunkList):
    for i in range(0,len(chunkList)-1):
        #print "Start " + str(chunkList[i])
        #print "End " + str(chunkList[i+1])
        if len(chunkList[i])==0:
            continue
        endValue = chunkList[i][len(chunkList[i])-1]
        begValue = chunkList[i+1][0]
        #print "start beg,end: " + str(begValue) + " " + str(endValue)
        while int(begValue) == int(endValue) and len(chunkList[i+1]) != 0:
            del chunkList[i+1][0]
            chunkList[i].append(begValue)
#    print "Alter Start " + str(chunkList[i])
#    print "Alter End " + str(chunkList[i+1])
            if len(chunkList[i+1]) == 0:
                continue
            begValue = chunkList[i+1][0]
        #    print "new begValue: " + str(begValue)
    for i in range(len(chunkList)-1,0,-1):
        if len(chunkList[i]) == 0:
            del chunkList[i]
    return len(chunkList)

def chunkUpList(valueList,numOfBins):

    ## If the number of unique values in this list is too small, adjust
    valueUnique = set(valueList)
    if numOfBins > len(valueUnique): numOfBins = len(valueUnique)

    minimumValue = min(valueList)
    maximumValue = max(valueList)
    numberOfValues = len(valueList)

    #Compute Initial Value for ideal chunk size
    idealChunk = int(round(float(numberOfValues)/float((numOfBins))))

    ## Create initial chunks
    chunkList = [valueList[i:i+idealChunk] for i in range(0,len(valueList),idealChunk)]

    ## If this produces more chunks than needed, adjust the chunks
    if len(chunkList) > numOfBins:
        appendList = chunkList.pop()
        
        for value in appendList:
            chunkList[len(chunkList)-1].append(value)

    ## Put like values in the same chunks
    beforeNumChunks = len(chunkList)
    afterNumChunks = 0
    while beforeNumChunks != afterNumChunks:
        beforeNumChunks = len(chunkList)
        afterNumChunks = compressList(chunkList)

    ## Return this tuple so that we can adjust our expectations
    return (numOfBins,chunkList)
    
    
def computeBoundaries(valueList,numOfBins):
    
    valueListSorted = sorted(valueList)

    ### Initial Chunks
    (orgNumBins,chunkList) = chunkUpList(valueListSorted,numOfBins)

    ### Iterate until we get to the proper number of bins
    while orgNumBins != len(chunkList):
        reducedNumBins = orgNumBins - 1
        newValueList = []
        for chunk in chunkList[1:]:
            for value in chunk:
                newValueList.append(value)

        (newNumBins,newChunkList) = chunkUpList(newValueList,reducedNumBins)

        if newNumBins+1 < orgNumBins: orgNumBins = newNumBins + 1

        chunkList = [chunkList[0]]
        for chunk in newChunkList:
            chunkList.append(chunk)

    # Create and Return Boundaries (Joel would not approve of the fact that this is multple lines :))
    boundaryList = []
    boundaryList.append((chunkList[0][0],chunkList[0][len(chunkList[0])-1]))	
    for i in range(1,len(chunkList)):
        boundaryList.append((chunkList[i-1][len(chunkList[i-1])-1]+1,chunkList[i][len(chunkList[i])-1]))

    return boundaryList

def computePercentBoundariesAndColors(maxBoundary = 25.0, scalar=100.0):
    ### First the colors, which is easy
    #if maxBoundary < 25:
    #    raise RuntimeError("maxBoundary must be 25 or larger")
    colors = ["255.255.0.0","255.255.199.0",
              "255.103.250.0",
              "255.15.249.167","255.17.140.255",
              "255.0.0.255"]
    percents = [0.05,0.10,0.15,0.25,0.50,1.0]
    boundaries = []
    if scalar <= 10:
	boundaries.append((1,1))
	boundaries.append((2,2))
	boundaries.append((3,3))
	boundaries.append((4,4))
	boundaries.append((5,5))
	boundaries.append((6,int(scalar)))
    elif scalar <= 30:
	boundaries.append((1,1))
	boundaries.append((2,2))
	boundaries.append((3,3))
	boundaries.append((4,4))
	boundaries.append((5,7))
	boundaries.append((8,int(scalar)))
    elif scalar <= 50:
	boundaries.append((1,1))
	boundaries.append((2,2))
	boundaries.append((3,3))
	boundaries.append((4,6))
	boundaries.append((6,10))
	boundaries.append((11,int(scalar)))
    elif scalar < 100:
	boundaries.append((1,1))
	boundaries.append((2,2))
	boundaries.append((3,5))
	boundaries.append((6,10))
	boundaries.append((11,25))
	boundaries.append((26,int(scalar)))    
    else:
	for i in range(0,len(percents)-1):
	    if i==0:
		lowerBoundary = 1
	    else:
		lowerBoundary = int(percents[i]*maxBoundary)

	    upperBoundary = int(percents[i+1]*maxBoundary)
	    if upperBoundary == lowerBoundary+1:
		    boundaries.append((lowerBoundary,lowerBoundary))
	    else:
		boundaries.append((lowerBoundary,upperBoundary-1))
	boundaries.append((int(maxBoundary),int(math.ceil(scalar))))
    
    return (colors,boundaries)
        
def computeColors(colorStart,colorEnd,numberOfColors):
    if isinstance(colorStart,str):
        colorStart = aBGR(value=colorStart)
    if isinstance(colorEnd,str):
        colorEnd = aBGR(value=colorEnd)
    colors = [colorStart]
    colorDiff = colorEnd - colorStart
    
    for i in range(1,numberOfColors-1):
        alpha = colorStart.alpha + i*int(math.ceil(colorDiff[0]/(numberOfColors-1)))
        blue = colorStart.blue + i*int(math.ceil(colorDiff[1]/(numberOfColors-1)))
        green = colorStart.green + i*int(math.ceil(colorDiff[2]/(numberOfColors-1)))
        red = colorStart.red + i*int(math.ceil(colorDiff[3]/(numberOfColors-1)))

        if alpha < 0: alpha = 0
        if blue < 0: blue = 0
        if green < 0: green = 0
        if red < 0: red = 0
        colors.append(aBGR(alpha,blue,green,red))
    colors.append(colorEnd)
    return colors

def main():
    test_color = aBGR(value="255.255.255.255")
    print "Test Color = %s"%str(test_color)
    print "Repr Color = \n%s"%repr(test_color)
    test_explicit_color = aBGR("125",234,128.0,45)
    print "Test Color = %s"%str(test_explicit_color)
    print "Repr Color = \n%s"%repr(test_explicit_color)

    valueList = [45,23,76,88,34,99,1,33,33,33,54,65,22,98,27,27,99,66]
    fiveBoundaries = computeBoundaries(valueList,5)
    threeBoundaries = computeBoundaries(valueList,3)
    oneBoundary = computeBoundaries(valueList,1)

    print "Testing Boundaries:"
    print "ValueList = " + str(valueList)
    print "Sorted = " + str(sorted(valueList))
    print "5 Boundaries = " + str(fiveBoundaries)
    print "3 Boundaries = " + str(threeBoundaries)
    print "1 Boundary = " + str(oneBoundary)

    valueList = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,4,5]
    fiveBoundaries = computeBoundaries(valueList,5)
    threeBoundaries = computeBoundaries(valueList,3)
    oneBoundary = computeBoundaries(valueList,1)
    print "ValueList = " + str(valueList)
    print "Sorted = " + str(sorted(valueList))
    print "5 Boundaries = " + str(fiveBoundaries)
    print "3 Boundaries = " + str(threeBoundaries)
    print "1 Boundary = " + str(oneBoundary)

    fiveColors = computeColors("255.255.0.0","255.0.0.255",5)
    tenColors = computeColors("255.255.0.0","255.0.0.255",10)
    print "5 colors :"
    for color in fiveColors:
        print str(color)
    print '10 colors:'
    for color in tenColors:
        print str(color)

    testLonLat = [LonLat() for i in range(3)]
    testLonLat[0].parseRec("1.0 1.0 1.0")
    testLonLat[1].parseRec("2.0 2.0 2.0:4")
    testLonLat[2].parseRec("3.0 3.0 3.0 1.2:3")
    
    print "Test LonLats: "
    for lonlat in testLonLat:
        print "\t%s"%(str(lonlat))

    testUSFips = [USFips() for i in range(3)]
    testUSFips[0].parseRec("st42.ct003 1.0")
    testUSFips[1].parseRec("st43.ct002 2.04342343413:2")
    testUSFips[2].parseRec("st45.ct005 3.0 3.0:3")

    print "Test USFips: "
    for usfips in testUSFips:
        print "\t%s"%(str(usfips))

    testLonLatLabels = [LonLatLabel() for i in range(3)]
    testLonLatLabels[0].parseRec('"Label 1" 1.0 1.0 -1')
    testLonLatLabels[1].parseRec('"Label &2" 2.0 2.0 2.0:2')
    testLonLatLabels[2].parseRec('"Lable /<>?L3" 3.0 3.0 3.0 3:3')

    print 'Test LonLatLabels:'
    for lonlatlabel in testLonLatLabels:
        print '\t%s'%(str(lonlatlabel))

    testLonLatPathRec = [["1 1.0 1.0 1.0:1","1 2.0 2.0 1.0:1","1 3.0 3.0 1.0:1"],
                         ["2 1.1 1.1 1.1 1:2","2 2.1 2.1 2.1 1:2"]]
    testLonLatPaths = [LonLatPath() for i in range(len(testLonLatPathRec))]
    testLonLatPaths[0].parseRec(testLonLatPathRec[0])
    testLonLatPaths[1].parseRec(testLonLatPathRec[1])

    print 'Test LonLatPath:'
    for llp in testLonLatPaths:
        print '\t%s'%(str(llp))
        
    testLonLatPolyRec = [["1 1.0 1.0 1.0:1","1 2.0 2.0 1.0:1","1 3.0 3.0 1.0:1","1 1.0 1.0 1.0:1"],
                         ["2 1.1 1.1 1.1 1:2","2 2.1 2.1 2.1 1:2","2 1.1 1.1 1.1 1:2"]]
    testLonLatPolys = [LonLatPoly() for i in range(len(testLonLatPolyRec))]
    testLonLatPolys[0].parseRec(testLonLatPolyRec[0])
    testLonLatPolys[1].parseRec(testLonLatPolyRec[1])
    print 'Test LonLatPoly:'
    for llp in testLonLatPolys:
        print '\t%s'%(str(llp))
    #test_plotInfo = PlotInfo(input_filename_="../../samples/CA-usfips.samp")
    #gaia = GAIA(test_plotInfo)
    #print test_plotInfo.output_filename
    #print test_plotInfo.printXMLMessage()
############
# Main hook
############

if __name__=="__main__":
    main()

    

        


    
