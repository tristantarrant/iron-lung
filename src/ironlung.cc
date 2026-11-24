// generated from file 'src/ironlung.dsp' by dsp2cc:
// Code generated with Faust 2.85.9 (https://faust.grame.fr)


namespace ironlung {

class Dsp: public PluginLV2 {
private:
	uint32_t fSampleRate;
	FAUSTFLOAT fHslider0;
	FAUSTFLOAT	*fHslider0_;
	double fRec2[2];
	FAUSTFLOAT fEntry0;
	FAUSTFLOAT	*fEntry0_;
	double fVec0[2];
	double fConst0;
	double fConst1;
	double fConst2;
	double fConst3;
	double fRec6[2];
	double fVec1[2];
	double fConst4;
	double fConst5;
	double fConst6;
	double fConst7;
	double fRec5[2];
	double fVec2[2];
	double fConst8;
	double fConst9;
	double fConst10;
	double fRec4[2];
	double fConst11;
	double fConst12;
	double fConst13;
	double fRec3[2];
	FAUSTFLOAT fHslider1;
	FAUSTFLOAT	*fHslider1_;
	double fRec7[2];
	double fConst14;
	double fRec0[2];
	FAUSTFLOAT fHslider2;
	FAUSTFLOAT	*fHslider2_;
	double fRec9[2];
	FAUSTFLOAT fHslider3;
	FAUSTFLOAT	*fHslider3_;
	double fRec11[2];
	double fConst15;
	double fRec12[2];
	double fVec3[2];
	double fConst16;
	double fConst17;
	double fConst18;
	double fRec10[2];
	double fVec4[2];
	double fConst19;
	double fRec8[2];

	void connect(uint32_t port,void* data);
	void clear_state_f();
	void init(uint32_t sample_rate);
	void compute(int count, FAUSTFLOAT *input0, FAUSTFLOAT *output0);

	static void clear_state_f_static(PluginLV2*);
	static void init_static(uint32_t sample_rate, PluginLV2*);
	static void compute_static(int count, FAUSTFLOAT *input0, FAUSTFLOAT *output0, PluginLV2*);
	static void del_instance(PluginLV2 *p);
	static void connect_static(uint32_t port,void* data, PluginLV2 *p);
public:
	Dsp();
	~Dsp();
};



Dsp::Dsp()
	: PluginLV2() {
	version = PLUGINLV2_VERSION;
	id = "ironlung";
	name = N_("Iron Lung");
	mono_audio = compute_static;
	stereo_audio = 0;
	set_samplerate = init_static;
	activate_plugin = 0;
	connect_ports = connect_static;
	clear_state = clear_state_f_static;
	delete_instance = del_instance;
}

Dsp::~Dsp() {
}

inline void Dsp::clear_state_f()
{
	for (int l0 = 0; l0 < 2; l0 = l0 + 1) fRec2[l0] = 0.0;
	for (int l1 = 0; l1 < 2; l1 = l1 + 1) fVec0[l1] = 0.0;
	for (int l2 = 0; l2 < 2; l2 = l2 + 1) fRec6[l2] = 0.0;
	for (int l3 = 0; l3 < 2; l3 = l3 + 1) fVec1[l3] = 0.0;
	for (int l4 = 0; l4 < 2; l4 = l4 + 1) fRec5[l4] = 0.0;
	for (int l5 = 0; l5 < 2; l5 = l5 + 1) fVec2[l5] = 0.0;
	for (int l6 = 0; l6 < 2; l6 = l6 + 1) fRec4[l6] = 0.0;
	for (int l7 = 0; l7 < 2; l7 = l7 + 1) fRec3[l7] = 0.0;
	for (int l8 = 0; l8 < 2; l8 = l8 + 1) fRec7[l8] = 0.0;
	for (int l9 = 0; l9 < 2; l9 = l9 + 1) fRec0[l9] = 0.0;
	for (int l10 = 0; l10 < 2; l10 = l10 + 1) fRec9[l10] = 0.0;
	for (int l11 = 0; l11 < 2; l11 = l11 + 1) fRec11[l11] = 0.0;
	for (int l12 = 0; l12 < 2; l12 = l12 + 1) fRec12[l12] = 0.0;
	for (int l13 = 0; l13 < 2; l13 = l13 + 1) fVec3[l13] = 0.0;
	for (int l14 = 0; l14 < 2; l14 = l14 + 1) fRec10[l14] = 0.0;
	for (int l15 = 0; l15 < 2; l15 = l15 + 1) fVec4[l15] = 0.0;
	for (int l16 = 0; l16 < 2; l16 = l16 + 1) fRec8[l16] = 0.0;
}

void Dsp::clear_state_f_static(PluginLV2 *p)
{
	static_cast<Dsp*>(p)->clear_state_f();
}

inline void Dsp::init(uint32_t sample_rate)
{
	fSampleRate = sample_rate;
	fConst0 = std::min<double>(1.92e+05, std::max<double>(1.0, static_cast<double>(fSampleRate)));
	fConst1 = 1.0 / std::tan(22.502250225022504 / fConst0);
	fConst2 = 1.0 - fConst1;
	fConst3 = 1.0 / (fConst1 + 1.0);
	fConst4 = std::tan(50.00000000000001 / fConst0);
	fConst5 = 1.0 / fConst4;
	fConst6 = 1.0 - fConst5;
	fConst7 = 1.0 / (fConst5 + 1.0);
	fConst8 = 1.0 / std::tan(27777.77777777778 / fConst0);
	fConst9 = 1.0 - fConst8;
	fConst10 = 1.0 / (fConst8 + 1.0);
	fConst11 = 1.0 / std::tan(1063.8297872340427 / fConst0);
	fConst12 = 1.0 - fConst11;
	fConst13 = 1.0 / (fConst11 + 1.0);
	fConst14 = 212765.95744680852 / fConst0;
	fConst15 = 925.925925925926 / fConst0;
	fConst16 = 1.0 / std::tan(177.3049645390071 / fConst0);
	fConst17 = 1.0 - fConst16;
	fConst18 = 1.0 / (fConst16 + 1.0);
	fConst19 = 0.8333333333333334 / fConst4;
	clear_state_f();
}

void Dsp::init_static(uint32_t sample_rate, PluginLV2 *p)
{
	static_cast<Dsp*>(p)->init(sample_rate);
}

void always_inline Dsp::compute(int count, FAUSTFLOAT *input0, FAUSTFLOAT *output0)
{
#define fHslider0 (*fHslider0_)
#define fEntry0 (*fEntry0_)
#define fHslider1 (*fHslider1_)
#define fHslider2 (*fHslider2_)
#define fHslider3 (*fHslider3_)
	double fSlow0 = 0.0010000000000000009 * static_cast<double>(fHslider0);
	int iSlow1 = static_cast<double>(fEntry0) > 0.5;
	double fSlow2 = ((iSlow1) ? 2.0 : 0.0);
	double fSlow3 = 0.0010000000000000009 * static_cast<double>(fHslider1);
	double fSlow4 = 0.0010000000000000009 * static_cast<double>(fHslider2);
	double fSlow5 = 0.0010000000000000009 * static_cast<double>(fHslider3);
	for (int i0 = 0; i0 < count; i0 = i0 + 1) {
		fRec2[0] = fSlow0 + 0.999 * fRec2[1];
		double fTemp0 = static_cast<double>(input0[i0]);
		fVec0[0] = fTemp0;
		fRec6[0] = -(fConst3 * (fConst2 * fRec6[1] - fConst1 * (fTemp0 - fVec0[1])));
		double fTemp1 = fRec6[0] / std::sqrt(std::sqrt(mydsp_faustpower4_f(0.2777777777777778 * std::fabs(fRec6[0])) + 1.0));
		fVec1[0] = fTemp1;
		fRec5[0] = -(fConst7 * (fConst6 * fRec5[1] - fConst5 * (fTemp1 - fVec1[1])));
		double fTemp2 = fRec5[0] * std::pow(1e+01, 0.05 * (fSlow2 + 5.7142857142857135 * (fRec0[1] + -4.85982869379015) + 18.0));
		double fTemp3 = std::fabs(fTemp2);
		double fTemp4 = ((iSlow1) ? fTemp2 / std::sqrt(std::sqrt(mydsp_faustpower4_f(0.25 * fTemp3) + 1.0)) : fTemp2 / std::sqrt(std::sqrt(mydsp_faustpower4_f(0.2777777777777778 * fTemp3) + 1.0)));
		fVec2[0] = fTemp4;
		fRec4[0] = -(fConst10 * (fConst9 * fRec4[1] - (fTemp4 + fVec2[1])));
		fRec3[0] = -(fConst13 * (fConst12 * fRec3[1] - fConst11 * (fRec4[0] - fRec4[1])));
		double fTemp5 = 0.955 * fRec3[0];
		fRec7[0] = fSlow3 + 0.999 * fRec7[1];
		fRec0[0] = std::min<double>(0.00019271948608137046 * (2e+04 * std::max<double>(0.0, std::min<double>(1.0, 0.1 * fRec2[0])) + 2.2e+04) + 0.62, std::max<double>(0.0, fRec0[1] + fConst14 * ((9.0 - fRec0[1]) / (2.5e+05 * mydsp_faustpower3_f(1.0 - std::max<double>(0.0, std::min<double>(1.0, 0.1 * fRec7[0]))) + 2.2e+04) - (std::min<double>(0.005, 0.00425531914893617 * std::max<double>(0.0, fTemp5 + -0.7)) + std::min<double>(0.005, 0.00425531914893617 * std::max<double>(0.0, -0.7 - fTemp5))))));
		fRec9[0] = fSlow4 + 0.999 * fRec9[1];
		fRec11[0] = fSlow5 + 0.999 * fRec11[1];
		double fTemp6 = std::max<double>(0.0005, std::min<double>(0.9995, 0.1 * fRec11[0]));
		double fTemp7 = 1.0 - fTemp6;
		double fTemp8 = 1.0 / fTemp6 + 1.0 / fTemp7;
		double fTemp9 = 0.23500000000000001 * fTemp8;
		double fTemp10 = fTemp9 + 0.5 / fTemp6 + 1.0;
		double fTemp11 = 1.0 / std::tan(fConst15 * (fTemp8 / fTemp10));
		fRec12[0] = -((fRec12[1] * (1.0 - fTemp11) - (fRec4[0] + fRec4[1])) / (fTemp11 + 1.0));
		double fTemp12 = fRec4[0] + (fRec4[0] - fRec12[0]) * ((fTemp9 + 0.5 / fTemp7 + 1.0) / fTemp10 + -1.0);
		fVec3[0] = fTemp12;
		fRec10[0] = -(fConst18 * (fConst17 * fRec10[1] - fConst16 * (fTemp12 - fVec3[1])));
		double fTemp13 = fRec10[0] * std::pow(std::max<double>(0.0, std::min<double>(1.0, 0.1 * fRec9[0])), 2.7);
		fVec4[0] = fTemp13;
		fRec8[0] = fConst7 * (fConst19 * (fTemp13 - fVec4[1]) - fConst6 * fRec8[1]);
		double fRec1 = fRec8[0] / std::sqrt(std::sqrt(mydsp_faustpower4_f(0.25641025641025644 * std::fabs(fRec8[0])) + 1.0));
		output0[i0] = static_cast<FAUSTFLOAT>(fRec1);
		fRec2[1] = fRec2[0];
		fVec0[1] = fVec0[0];
		fRec6[1] = fRec6[0];
		fVec1[1] = fVec1[0];
		fRec5[1] = fRec5[0];
		fVec2[1] = fVec2[0];
		fRec4[1] = fRec4[0];
		fRec3[1] = fRec3[0];
		fRec7[1] = fRec7[0];
		fRec0[1] = fRec0[0];
		fRec9[1] = fRec9[0];
		fRec11[1] = fRec11[0];
		fRec12[1] = fRec12[0];
		fVec3[1] = fVec3[0];
		fRec10[1] = fRec10[0];
		fVec4[1] = fVec4[0];
		fRec8[1] = fRec8[0];
	}
#undef fHslider0
#undef fEntry0
#undef fHslider1
#undef fHslider2
#undef fHslider3
}

void __rt_func Dsp::compute_static(int count, FAUSTFLOAT *input0, FAUSTFLOAT *output0, PluginLV2 *p)
{
	static_cast<Dsp*>(p)->compute(count, input0, output0);
}


void Dsp::connect(uint32_t port,void* data)
{
	switch ((PortIndex)port)
	{
	case ATTACK: 
		fHslider1_ = (float*)data; // , 5.0, 0.0, 1e+01, 0.01 
		break;
	case LEVEL: 
		fHslider2_ = (float*)data; // , 5.0, 0.0, 1e+01, 0.01 
		break;
	case SUSTAIN: 
		fHslider0_ = (float*)data; // , 5.0, 0.0, 1e+01, 0.01 
		break;
	case TONE: 
		fHslider3_ = (float*)data; // , 5.0, 0.0, 1e+01, 0.01 
		break;
	case VCA: 
		fEntry0_ = (float*)data; // , 0.0, 0.0, 1.0, 1.0 
		break;
	default:
		break;
	}
}

void Dsp::connect_static(uint32_t port,void* data, PluginLV2 *p)
{
	static_cast<Dsp*>(p)->connect(port, data);
}


PluginLV2 *plugin() {
	return new Dsp();
}

void Dsp::del_instance(PluginLV2 *p)
{
	delete static_cast<Dsp*>(p);
}

/*
typedef enum
{
   ATTACK, 
   LEVEL, 
   SUSTAIN, 
   TONE, 
   VCA, 
} PortIndex;
*/

} // end namespace ironlung
