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

func Exit(value int)  {
    os.Exit(value)
}

func Check(err error) {
    if err != nil {
        fmt.Printf("Error: %s\n", err)
        Exit(1)
    }
}

func URL_Get(url string) string {
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
func Latest_Version(data string, major_start int, major_end int) string {
    var found string = "";
    for major := major_end; major >= major_start; major-- {
        for minor := 0; minor < 10; minor++ {
            tmp := fmt.Sprintf("%d.%d", major, minor)
            if strings.Contains(data, tmp) {
                found = tmp;
                return found;
            }
        }
    }
    return "";
}

func Debian_Latest(arch string) (string, string) {
    base_url := "http://cdimage.debian.org/debian-cd/current/multi-arch/iso-cd";

    data := URL_Get(base_url)

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

func FreeBSD_Latest(arch string) (string, string) {

    base_url := "http://ftp.freebsd.org/pub/FreeBSD/releases/ISO-IMAGES";

    data := URL_Get(base_url)

    var version string = Latest_Version(data, FREEBSD_MAJOR_START, FREEBSD_MAJOR_END)

    if version != "" {
        return fmt.Sprintf("%s/%s/FreeBSD-%s-RELEASE-%s-memstick.img", base_url, version, version, arch), version;
    }

    return "", version;
}

func OpenBSD_Latest(arch string) (string, string) {
    base_url := "http://mirror.ox.ac.uk/pub/OpenBSD";
 
    data := URL_Get(base_url) 

    var version string  = Latest_Version(data, OPENBSD_MAJOR_START, OPENBSD_MAJOR_END)

    // hack for changing 6.0 to 60 for filename on server
    img_ver := strings.Replace(version, ".", "", -1)
    
    if version != "" {
        return fmt.Sprintf("%s/%s/%s/install%s.fs", base_url, version, arch, img_ver), version;
    }

    return "", ""
}

func Latest_URL(os string, arch string) (string, string) {
    var url string = ""
    var version string = ""
    
    switch (os) {
        case "OpenBSD":
        url, version = OpenBSD_Latest(arch)
        break;

        case "FreeBSD":
        url, version = FreeBSD_Latest(arch)
        break;

        case "Debian":
        url, version = Debian_Latest(arch)
        break;
    }

    if len(url) == 0 {
        fmt.Printf("Error: Latest_URL %s %s\n", os, arch)
        Exit(1)
    }

    return url, fmt.Sprintf("%s %s (%s)", os, version, arch);
}

func Distro_Find_All() map[string]string {

    distros := make(map[string]string)

    url, version := Latest_URL("OpenBSD", "i386");
    distros[version] = url;
    
    url, version = Latest_URL("OpenBSD", "amd64");
    distros[version] = url;

    url, version = Latest_URL("FreeBSD", "i386");
    distros[version] = url;

    url, version = Latest_URL("FreeBSD", "amd64");
    distros[version] = url;

    url, version = Latest_URL("Debian", "i386/amd64");
    distros[version] = url;
 
    return distros;
}

func main() {

    if len(os.Args) != 2 {
        fmt.Println("You must provide a file to write output to!")
        Exit(1)
    }

    file := os.Args[1];

    distros := Distro_Find_All()

    f, err := os.Create(file)
    Check(err)
    defer f.Close()

    w := bufio.NewWriter(f)
    
    for name, url := range distros {
        fmt.Fprintf(w, "%s=%s\n", name, url)
        //fmt.Printf("Adding: %s with URL: %s\n", name, url)
    }

    fmt.Printf("Saved to %s", file)
    w.Flush()

    Exit(0)
}
