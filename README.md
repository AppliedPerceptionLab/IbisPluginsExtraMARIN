# IbisPluginsExtraMARIN
These plugins are complementary to the [MARIN app](https://github.com/AppliedPerceptionLab/MARIN) to integrate with the [IbisNeuronav platform](https://github.com/IbisNeuronav/Ibis) and enable in situ AR guidance on a mobile device. They were developed by [Étienne Léger](https://ap-lab.ca/people/etienneleger/), at the [Applied Perception Lab](https://ap-lab.ca/).

If you use MARIN or these plugins in your research, cite: 
> Léger, É., Reyes, J., Drouin, S., Popa, T., Hall, J. A., Collins, D. L., Kersten-Oertel, M., "MARIN: an Open Source Mobile Augmented Reality Interactive Neuronavigation System", International Journal of Computer Assisted Radiology and Surgery (2020). https://doi.org/10.1007/s11548-020-02155-6

DISCLAIMER: MARIN and these plugins are a research tool: It is not intended for normal clinical use, and is not FDA nor CE approved.

## Build Instructions:  

This code should simply be checked out inside Ibis' root directory. These plugins should then be available in the CMake configuration, and can be added through variables IBIS_PLUGINS_BUILD_CommandsProcessing and IBIS_PLUGINS_BUILD_OpenIGTLinkSender. 

### Dependencies  
* [OpenIGTLink](https://github.com/openigtlink/OpenIGTLink) (or through the IGSIO_Hardware_Module in Ibis, which builds OpenIGTLink)  
* [OpenIGTLinkIO](https://github.com/errollgarner/OpenIGTLinkIO/tree/UDP_support)
