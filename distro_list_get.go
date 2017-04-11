package main;

import (
    "fmt"
    "os"
    "strings"
    "net/http"
    "io/ioutil"
    "bufio"
)

const FREEBSD_MAJOR_START = 11
const FREEBSD_MAJOR_END = 20
const OPENBSD_MAJOR_START = 6;
const OPENBSD_MAJOR_END = 10;

type searchFunc func(string, string) (string, string)
type distro_t struct {
    name string;
    arch string;
    url  string;
    search searchFunc;
}

var distributions = []distro_t {
	{ name: "Debian", arch: "i386/amd64", search: LatestDebian, url: "http://cdimage.debian.org/debian-cd/current/multi-arch/iso-cd" },
	{ name: "OpenBSD", arch: "i386", search: LatestOpenBSD, url: "http://mirror.ox.ac.uk/pub/OpenBSD" },
	{ name: "OpenBSD", arch: "amd64", search: LatestOpenBSD, url: "http://mirror.ox.ac.uk/pub/OpenBSD" },
        { name: "OpenSUSE", arch: "x86_64", search: LatestOpenSUSE, url: "http://download.opensuse.org/tumbleweed/iso/openSUSE-Tumbleweed-DVD-x86_64-Current.iso" },
	{ name: "FreeBSD", arch: "i386", search: LatestFreeBSD, url: "http://ftp.freebsd.org/pub/FreeBSD/releases/ISO-IMAGES" },
	{ name: "FreeBSD", arch: "amd64", search: LatestFreeBSD, url: "http://ftp.freebsd.org/pub/FreeBSD/releases/ISO-IMAGES"  },
}

func Exit(value int)  {
    os.Exit(value)
}

func Check(err error) {
    if err != nil {
        fmt.Printf("Error: %s\n", err)
        Exit(1)
    }
}

func GetURL(url string) string {
    req, err := http.Get(url)
    Check(err)
    defer req.Body.Close()

    if req.StatusCode != http.StatusOK {
        return "";
    }

    bytes, err := ioutil.ReadAll(req.Body)
    Check(err)

    return string(bytes)
}

// returns when highest value version is found...
func LatestVersionGet(data string, major_start int, major_end int) string {
    var found string = "";
    for major := major_end; major >= major_start; major-- {
        for minor := 9; minor >= 0; minor-- {
            tmp := fmt.Sprintf("%d.%d", major, minor)
            if strings.Contains(data, tmp) {
                found = tmp;
                return found;
            }
        }
    }
    return "";
}

func LatestDebian(base_url string, arch string) (string, string) {

    data := GetURL(base_url)

    tag_start := "<a href=\"";
    tag_full := tag_start + "debian-"
    idx := strings.Index(data, tag_full)
    str := data[idx + len(tag_start) :len(data)];
    end := strings.Index(str, "\"") 
    version_start := data[idx + len(tag_full):len(data)];
    version_end := strings.Index(version_start, "-");
    version := version_start[0:version_end];
    filename := str[0:end]

    return fmt.Sprintf("%s/%s", base_url, filename), version;
}

func LatestFreeBSD(base_url string, arch string) (string, string) {

    data := GetURL(base_url)

    var version string = LatestVersionGet(data, FREEBSD_MAJOR_START, FREEBSD_MAJOR_END)

    if version != "" {
        return fmt.Sprintf("%s/%s/FreeBSD-%s-RELEASE-%s-memstick.img", base_url, version, version, arch), version;
    }

    return "", version;
}

func LatestOpenSUSE(base_url string, arch string) (string, string) {
	return base_url, "Tumbleweed"
}

func LatestOpenBSD(base_url string, arch string) (string, string) {

    data := GetURL(base_url) 

    var version string  = LatestVersionGet(data, OPENBSD_MAJOR_START, OPENBSD_MAJOR_END)

    // hack for changing 6.0 to 60 for filename on server
    img_ver := strings.Replace(version, ".", "", -1)

    if version != "" {
        return fmt.Sprintf("%s/%s/%s/install%s.fs", base_url, version, arch, img_ver), version;
    }

    return "", ""
}

func DistroFindAll() map[string]string {

    distros := make(map[string]string)

    for _, info := range distributions {
	url, version := info.search(info.url, info.arch);
	release_name := fmt.Sprintf("%s %s (%s)", info.name, version, info.arch);
	distros[release_name] = url;
    }

    return distros;
}

func main() {

    if len(os.Args) != 2 {
        fmt.Println("You must provide a file to write output to!")
        Exit(1)
    }

    file := os.Args[1];

    distros := DistroFindAll()

    f, err := os.Create(file)
    Check(err)
    defer f.Close()

    w := bufio.NewWriter(f)

    for name, url := range distros {
        fmt.Fprintf(w, "%s=%s\n", name, url)
        //fmt.Printf("Adding: %s with URL: %s\n", name, url)
    }

    fmt.Printf("Saved to %s\n", file)
    w.Flush()

    Exit(0)
}
