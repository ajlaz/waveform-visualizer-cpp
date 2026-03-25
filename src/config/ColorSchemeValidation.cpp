#include "ColorSchemeValidation.h"

static bool validateColor3(const nlohmann::json &j, const char *key,
                           const std::string &path,
                           std::vector<std::string> &errors)
{
    if (!j.contains(key))
    {
        errors.push_back(path + "." + key + " is missing");
        return false;
    }
    const auto &v = j.at(key);
    if (!v.is_string())
    {
        errors.push_back(path + "." + key + " must be a hex color string like \"#RRGGBB\"");
        return false;
    }
    const std::string s = v.get<std::string>();
    if (s.size() != 7 || s[0] != '#')
    {
        errors.push_back(path + "." + key + " must be in format \"#RRGGBB\"");
        return false;
    }
    for (size_t i = 1; i < s.size(); ++i)
    {
        const char c = s[i];
        const bool isHex = (c >= '0' && c <= '9') ||
                           (c >= 'a' && c <= 'f') ||
                           (c >= 'A' && c <= 'F');
        if (!isHex)
        {
            errors.push_back(path + "." + key + " must be valid hex digits");
            return false;
        }
    }
    return true;
}

static bool validateFloatRange(const nlohmann::json &j, const char *key,
                               float minVal, float maxVal,
                               const std::string &path,
                               std::vector<std::string> &errors)
{
    if (!j.contains(key))
    {
        errors.push_back(path + "." + key + " is missing");
        return false;
    }
    const auto &v = j.at(key);
    if (!v.is_number())
    {
        errors.push_back(path + "." + key + " must be a number");
        return false;
    }
    const float f = v.get<float>();
    if (f < minVal || f > maxVal)
    {
        errors.push_back(path + "." + key + " must be in [" +
                         std::to_string(minVal) + "," + std::to_string(maxVal) + "]");
        return false;
    }
    return true;
}

static void validateSectionObject(const nlohmann::json &root, const char *key,
                                  std::vector<std::string> &errors)
{
    if (!root.contains(key) || !root.at(key).is_object())
    {
        errors.push_back(std::string(key) + " section is missing or not an object");
    }
}

bool validateColorSchemeJson(const nlohmann::json &root,
                             std::vector<std::string> &errors)
{
    validateSectionObject(root, "oscilloscope", errors);
    validateSectionObject(root, "spectrum", errors);
    validateSectionObject(root, "spectrogram", errors);
    validateSectionObject(root, "vu_meter", errors);
    validateSectionObject(root, "waveform", errors);
    validateSectionObject(root, "stereo_imager", errors);

    if (!errors.empty())
    {
        return false;
    }

    const auto &osc = root.at("oscilloscope");
    validateColor3(osc, "line", "oscilloscope", errors);

    const auto &spec = root.at("spectrum");
    validateColor3(spec, "low", "spectrum", errors);
    validateColor3(spec, "mid", "spectrum", errors);
    validateColor3(spec, "high", "spectrum", errors);
    validateColor3(spec, "background", "spectrum", errors);
    validateColor3(spec, "tip", "spectrum", errors);

    const auto &spectro = root.at("spectrogram");
    validateColor3(spectro, "c0", "spectrogram", errors);
    validateColor3(spectro, "c1", "spectrogram", errors);
    validateColor3(spectro, "c2", "spectrogram", errors);
    validateColor3(spectro, "c3", "spectrogram", errors);
    validateColor3(spectro, "c4", "spectrogram", errors);
    validateFloatRange(spectro, "t1", 0.0f, 1.0f, "spectrogram", errors);
    validateFloatRange(spectro, "t2", 0.0f, 1.0f, "spectrogram", errors);
    validateFloatRange(spectro, "t3", 0.0f, 1.0f, "spectrogram", errors);
    validateFloatRange(spectro, "t4", 0.0f, 1.0f, "spectrogram", errors);
    if (errors.empty())
    {
        const float t1 = spectro.at("t1").get<float>();
        const float t2 = spectro.at("t2").get<float>();
        const float t3 = spectro.at("t3").get<float>();
        const float t4 = spectro.at("t4").get<float>();
        if (!(t1 < t2 && t2 < t3 && t3 < t4))
        {
            errors.push_back("spectrogram thresholds must be strictly increasing (t1 < t2 < t3 < t4)");
        }
    }

    const auto &vu = root.at("vu_meter");
    validateColor3(vu, "background", "vu_meter", errors);
    validateColor3(vu, "unfilled", "vu_meter", errors);
    validateColor3(vu, "green", "vu_meter", errors);
    validateColor3(vu, "yellow", "vu_meter", errors);
    validateColor3(vu, "red", "vu_meter", errors);
    validateColor3(vu, "peak", "vu_meter", errors);
    validateColor3(vu, "border", "vu_meter", errors);
    validateColor3(vu, "ticks", "vu_meter", errors);

    const auto &wf = root.at("waveform");
    validateColor3(wf, "low", "waveform", errors);
    validateColor3(wf, "mid", "waveform", errors);
    validateColor3(wf, "high", "waveform", errors);
    validateFloatRange(wf, "colorGain", 0.0f, 2.0f, "waveform", errors);
    validateFloatRange(wf, "mixGain", 0.0f, 2.0f, "waveform", errors);

    if (root.contains("stereo_imager") && root.at("stereo_imager").is_object()) {
        const auto &si = root.at("stereo_imager");
        validateColor3(si, "background", "stereo_imager", errors);
        validateColor3(si, "dot",        "stereo_imager", errors);
    }

    return errors.empty();
}
