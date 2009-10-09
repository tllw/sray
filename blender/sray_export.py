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

import Blender

class Triangle:
	__slots__ = 'vidx', 'tc'
	def __init__(self, vidx, tc):
		self.vidx = vidx
		self.tc = tc

def export(fname):
	scene = Blender.Scene.GetCurrent()
	objects = scene.objects
	
	try:
		file = open(fname, "w")
	except Exception, e:
		print "failed to open file:", fname, ":", e
	
	file.write("<scene>\n")
	file.write("\t<!-- Exported from blender by sray_export.py (sray exporter) version %s -->\n" % __version__)
	file.write("\t<!-- The sray exporter is part of the s-ray renderer project: http://code.google.com/p/sray -->\n")
	
	for obj in objects:
		odata = obj.getData(mesh = True)
		
		if obj.type == 'Mesh':
			file.write("\t<object name=\"%s\" type=\"mesh\">\n" % obj.name)
			write_xform(file, obj)
			write_mesh(file, odata)
			file.write("\t</object>\n")
			
	file.write("</scene>\n")

			
# might want to add time ...
def write_xform(file, obj):
	# might want to use those repetedly with "worldpsace" arg to export anim
	x, y, z = obj.getLocation()
	#q = obj.getEuler().toQuat() # this gives me bollocks for some reason
	sx, sy, sz = obj.getSize()

	m = obj.getMatrix()
	q = m.toQuat()
		
	file.write("\t\t<xform pos=\"%f %f %f\" rot=\"%f %f %f %f\" scale=\"%f %f %f\"/>\n" % (x, z, y, q.w, q.x, q.z, q.y, sx, sz, sy))

	#file.write("\t\t<!-- matrix -->\n")
	#file.write("\t\t<!-- %.3f %.3f %.3f -->\n" % (m[0][0], m[0][1], m[0][2]))
	#file.write("\t\t<!-- %.3f %.3f %.3f -->\n" % (m[1][0], m[1][1], m[1][2]))
	#file.write("\t\t<!-- %.3f %.3f %.3f -->\n" % (m[2][0], m[2][1], m[2][2]))

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
			
			file.write("\t\t\t<vertex id=\"%d\" val=\"%f %f %f\"/>\n" % (vidx, v.co.x, v.co.z, v.co.y))
			file.write("\t\t\t<normal id=\"%d\" val=\"%f %f %f\"/>\n" % (vidx, v.no.x, v.no.z, v.no.y))
			file.write("\t\t\t<texcoord id=\"%d\" val=\"%f %f\"/>\n" % (vidx, tc[0], tc[1]))
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

# run exporter with a fixed filename for now (TODO: impl. file selector)
export("/tmp/export.xml")
