#!BPY
""" 
Name: 'S-ray ...'
Blender: 243
Group: 'Export'
Tooltip: 'Export to s-ray XML scene description format.'
"""

__author__ = ["Ioannis Tsiompikas"]
__url__ = ("code.google.com/p/sray")
__version__ = "0.001"

import math
import Blender
from Blender import Mathutils
from Blender import Texture,Image,Material,Ipo

class Triangle:
	__slots__ = 'vidx', 'tc'
	def __init__(self, vidx, tc):
		self.vidx = vidx
		self.tc = tc

def export(fname):
	scene = Blender.Scene.GetCurrent()

	print "-- sray exporter version", __version__
	print "exporting scene ->", fname

	fps = scene.getRenderingContext().framesPerSec()
	anim_start = Blender.Get('staframe') / fps
	anim_end = Blender.Get('endframe') / fps

	print "fps: ", fps
	print "animation: ", anim_start, "->", anim_end
	
	try:
		file = open(fname, "w")
	except Exception, e:
		print "failed to open file:", fname, ":", e
	
	file.write("<scene>\n")
	file.write("\t<!-- Exported from blender by sray_export.py (sray exporter) version %s -->\n" % __version__)
	file.write("\t<!-- The sray exporter is part of the s-ray renderer project: http://code.google.com/p/sray -->\n")

	# write all materials
	for obj in scene.objects:
		if obj.type == 'Mesh':
			mesh = obj.getData(mesh = True)
			mat = mesh.materials[0]
			matname = obj.name + ".mat"

			if mat != None:
				ifname = None
				if mesh.faceUV:
					img = mesh.faces[0].image
					if img != None:
						ifname = img.filename.split('\\')[-1].split('/')[-1]
				write_material(file, mat, matname, ifname)
			else:
				file.write("\t<material name=\"%s\" shader=\"phong\"/>\n")

	# export meshes, lights, and cameras
	for obj in scene.objects:
		if obj.type == 'Mesh':
			write_object(file, obj)

		if obj.type == 'Lamp':
			write_light(file, obj)

		if obj.type == 'Camera':
			write_cam(file, obj)
			
	file.write("</scene>\n")

def write_object(file, obj):
	print "- object:", obj.name
	mesh = obj.getData(mesh = True)

	file.write("\t<object name=\"%s\" type=\"mesh\">\n" % obj.name)
	file.write("\t\t<matref name=\"%s\"/>\n" % (obj.name + ".mat"))

	ipo = obj.getIpo()
	if ipo == None:
		write_xform(file, obj)
	else:
		write_anim(file, ipo)
	
	write_mesh(file, mesh)
	file.write("\t</object>\n")
			
# might want to add time ...
def write_xform(file, obj, rot=False):
	print "\txform"
	# might want to use those repetedly with "worldpsace" arg to export anim
	x, y, z = obj.getLocation()
	#q = obj.getEuler().toQuat() # this gives me bollocks for some reason
	sx, sy, sz = obj.getSize()

	if rot:
		rmat = Mathutils.Matrix([1, 0, 0, 0],
								[0, 0, -1, 0],
								[0, 1, 0, 0],
								[0, 0, 0, 1])
		m = rmat * obj.getMatrix()
	else:
		m = obj.getMatrix()
	q = m.toQuat()

	#INVERT-YZ
	file.write("\t\t<xform pos=\"%f %f %f\" rot=\"%f %f %f %f\" scale=\"%f %f %f\"/>\n" %
			(x, z, y, q.w, q.x, q.z, q.y, sx, sz, sy))

	#file.write("\t\t<!-- matrix -->\n")
	#file.write("\t\t<!-- %.3f %.3f %.3f -->\n" % (m[0][0], m[0][1], m[0][2]))
	#file.write("\t\t<!-- %.3f %.3f %.3f -->\n" % (m[1][0], m[1][1], m[1][2]))
	#file.write("\t\t<!-- %.3f %.3f %.3f -->\n" % (m[2][0], m[2][1], m[2][2]))

def write_anim(file, ipo, rot=False):
	print "\tanim"

	scn = Blender.Scene.GetCurrent()
	fps = scn.getRenderingContext().framesPerSec()
	anim_start = Blender.Get('staframe') / fps
	anim_end = Blender.Get('endframe') / fps

	curve_posx = ipo[Ipo.OB_LOCX]
	curve_posy = ipo[Ipo.OB_LOCY]
	curve_posz = ipo[Ipo.OB_LOCZ]
	curve_rotx = ipo[Ipo.OB_ROTX]
	curve_roty = ipo[Ipo.OB_ROTY]
	curve_rotz = ipo[Ipo.OB_ROTZ]
	curve_sclx = ipo[Ipo.OB_SCALEX]
	curve_scly = ipo[Ipo.OB_SCALEY]
	curve_sclz = ipo[Ipo.OB_SCALEZ]

	anim_dur = anim_end - anim_start
	sam_per_sec = 1.0
	num_samples = anim_dur / sam_per_sec
	t = anim_start
	dt = 1.0 / sam_per_sec

	print "anim_dur =", anim_dur
	print "num_samples =", num_samples

	for i in range(int(num_samples)):
		frame = t * fps
		x = curve_posx[frame]
		y = curve_posy[frame]
		z = curve_posz[frame]
		rx = curve_rotx[frame]
		ry = curve_roty[frame]
		rz = curve_rotz[frame]
		sx = curve_sclx[frame]
		sy = curve_scly[frame]
		sz = curve_sclz[frame]

		m = Mathutils.Euler(rx * 10.0, ry * 10.0, rz * 10.0).toMatrix()

		if rot:
			rmat = Mathutils.Matrix([1, 0, 0, 0], [0, 0, -1, 0], [0, 1, 0, 0], [0, 0, 0, 1])
			m = rmat * m
		q = m.toQuat()

		print "animation sample", t, "->", z
		file.write("\t\t<xform time=\"%d\" pos=\"%f %f %f\" rot=\"%f %f %f %f\" scale=\"%f %f %f\"/>\n" %
				(int(t * 1000.0), x, z, y, q.w, q.x, q.z, q.y, sx, sz, sy))
		t += dt


def write_material(file, mat, name, diff_tex = None):
	texlist = mat.getTextures()

	print "material:", name
	if diff_tex != None:
		print "  tex:", diff_tex
#	for tex in texlist:
#		if tex != None:
#			print tex.mapto
#		else:
#			print "[no tex]"


	sdr = "phong"	# TODO change this when the renderer supports more shaders
	file.write("\t<material name=\"%s\" shader=\"%s\">\n" % (name, sdr))
	write_mattr(file, "diffuse", tuple([c * mat.ref for c in mat.rgbCol]), diff_tex)
	write_mattr(file, "specular", tuple([c * mat.spec for c in mat.specCol]))
	write_mattr(file, "shininess", ((mat.getHardness() - 1) * 1.9607843137254901))
	write_mattr(file, "ior", mat.IOR)
	write_mattr(file, "reflect", mat.rayMirr)
	write_mattr(file, "refract", 1.0 - mat.alpha)
	file.write("\t</material>\n")


def write_mattr(file, name, val, tex = None):
	file.write("\t\t<mattr name=\"%s\" " % name)
	
	if type(val) == tuple:
		if len(val) == 3:
			file.write("value=\"%f %f %f\" " % (val[0], val[1], val[2]))
		elif len(val) >= 4:
			file.write("value=\"%f %f %f %f\" " % (val[0], val[1], val[2], val[3]))
	else:
		file.write("value=\"%f\" " % val)

	if tex != None:
		file.write("map=\"%s\"" % tex)

	file.write("/>\n")


def write_mesh(file, m):
	file.write("\t\t<mesh>\n")

	trilist = []

	for f in m.faces:
		have_tc = False
		if m.faceUV:
			have_tc = True

		tc = [[0, 0], [0, 0], [0, 0]]

		if len(f.v) == 3:
			# if it's a triangle just add it to the list
			if have_tc:
				tc = [f.uv[0], f.uv[1], f.uv[2]]
			tri = Triangle((f.v[0].index, f.v[1].index, f.v[2].index), tc)
			trilist.append(tri)

		else:
			# if it's a quad, split it into two triangles first...
			if have_tc:
				tc = [f.uv[0], f.uv[1], f.uv[2]]
			tri = Triangle((f.v[0].index, f.v[1].index, f.v[2].index), tc)
			trilist.append(tri)
			
			if have_tc:
				tc = [f.uv[0], f.uv[2], f.uv[3]]
			tri = Triangle((f.v[0].index, f.v[2].index, f.v[3].index), tc)
			trilist.append(tri)

	tri_idx = 0
	for tri in trilist:
		for i in range(3):
			v = m.verts[tri.vidx[i]]
			tc = tri.tc[i]

			vidx = tri_idx * 3 + i
			
			#INVERT-YZ
			file.write("\t\t\t<vertex id=\"%d\" val=\"%f %f %f\"/>\n" % (vidx, v.co.x, v.co.z, v.co.y))
			file.write("\t\t\t<normal id=\"%d\" val=\"%f %f %f\"/>\n" % (vidx, v.no.x, v.no.z, v.no.y))
			file.write("\t\t\t<texcoord id=\"%d\" val=\"%f %f\"/>\n" % (vidx, tc[0], -tc[1]))
		tri_idx += 1

	tri_idx = 0
	for tri in trilist:
		file.write("\t\t\t<face id=\"%d\">\n" % tri_idx)

		for i in range(3):
			idx = tri_idx * 3 + i
			file.write("\t\t\t\t<vref vertex=\"%d\" normal=\"%d\" texcoord=\"%d\"/>\n" % (idx, idx, idx))

		file.write("\t\t\t</face>\n")
		tri_idx += 1

	file.write("\t\t</mesh>\n")


def rad2deg(x):
	return 180.0 * x / math.pi 

def write_cam(file, obj):
	print "- camera:", obj.name
	cam = obj.getData()
	fov = rad2deg(math.atan(16.0 / cam.lens) * 2.0) / 1.333333

	file.write("\t<camera name=\"%s\" type=\"free\" fov=\"%.3f\">\n" % (cam.name, fov))
	write_xform(file, obj, True)
	file.write("\t</camera>\n")


def write_light(file, obj):
	print "- light:", obj.name
	lt = obj.getData()

	r = lt.col[0]
	g = lt.col[1]
	b = lt.col[2]

	ltype = "point"
	if lt.type == 'Area':
		if lt.mode & lt.Modes['Sphere']:
			ltype = "sphere"
		elif lt.mode & lt.Modes['Square']:
			ltype = "box"	

	file.write("\t<light name=\"%s\" type=\"%s\" color=\"%f %f %f\">\n" % (obj.name, ltype, r, g, b))
	write_xform(file, obj)

	if ltype == "sphere":
		file.write("\t\t<sphere rad=\"%f\"/>\n", lt.areaSizeX)
	if ltype == "box":
		minx = -lt.areaSizeX / 2.0
		miny = -lt.areaSizeY / 2.0
		maxx = lt.areaSizeX / 2.0
		maxy = lt.areaSizeY / 2.0
		file.write("\t\t<aabox min=\"%f 0 %f\" max=\"%f 0 %f\"/>\n" % (minx, miny, maxx, maxy))

	file.write("\t</light>\n")

# run exporter with a fixed filename for now (TODO: impl. file selector)
export("/tmp/export.xml")
