**Update:** It's highly unlikely that I'll continue developing s-ray in the future. A new renderer project is underway: [calacyria](https://github.com/MutantStargoat/calacirya).

S-ray is a free photorealistic renderer based on monte carlo ray tracing.

It is capable of simulating all interactions of light with the environment, or L(D|S)`*`E paths in path notation. What this means is that it can simulate global illumination effects such as indirect diffuse illumination, caustics, and accurate reflection and refraction.

![![](http://farm4.static.flickr.com/3512/3978847546_1869d67959_m_d.jpg)](http://farm4.static.flickr.com/3512/3978847546_7b65677ddf_o.jpg)
![![](http://farm4.static.flickr.com/3533/3978847480_7fa542ef6b_o.jpg)](http://farm3.static.flickr.com/2667/3978085537_8646271c59_o.jpg)
![![](http://farm3.static.flickr.com/2442/3978847094_6b8cff209e_o.jpg)](http://farm3.static.flickr.com/2564/3978847042_fa8dcf8a6b_o.jpg)

![![](http://farm3.static.flickr.com/2497/3978846894_58bfabf3c5_o.jpg)](http://farm4.static.flickr.com/3477/3978846990_5aa1d290b4_o.jpg)
![![](http://farm3.static.flickr.com/2502/3978085965_eb685bbae0_o.jpg)](http://farm3.static.flickr.com/2600/3978847300_1c2160d690_o.jpg)
![![](http://farm3.static.flickr.com/2594/3978847592_2f8973b353_o.jpg)](http://farm3.static.flickr.com/2547/3978085753_72bac1a1bd_o.jpg)

Here is a more or less complete feature list:
  * Indirect diffuse illumination and caustics.
  * Reflection and refraction from perfect and imperfect (glossy) surfaces.
  * Area lights casting shadows with penumbra regions (soft shadows).
  * Motion blur.
  * Adaptive supersampling.
  * Hierarchical keyframe animation.
  * Simple XML scene description format, with converters from popular 3D file formats.

A lot more features are planned for the near future, check out the ToDo list.

To start playing around with s-ray see: GettingStarted