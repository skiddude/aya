<script>
    import { onMount } from "svelte"

    let { transport } = $props()

    let platform = $state(null)
    let version = $state(null)
    let compiler = $state(null)
    let compileTimestamp = $state(null)
    let isUsingInstance = $state(null)
    let instanceName = $state(null)
    let instanceMotd = $state(null)
    let instanceUrl = $state(null)
    let instanceUpdateState = $state(null)
    let totalPlaytime = $state(null)

    let totalItems = $state(0)
    let totalModels = $state(0)
    let totalLevels = $state(0)

    const dependencies = [
        { name: "assimp", license: "https://github.com/assimp/assimp/blob/master/LICENSE" },
        { name: "BGFX", license: "https://github.com/bkaradzic/bgfx/blob/master/LICENSE" },
        { name: "Boost", license: "https://www.boost.org/users/license.html" },
        { name: "Bullet Physics SDK", license: "https://github.com/bulletphysics/bullet3/blob/master/LICENSE.txt" },
        { name: "cURL", license: "https://curl.se/docs/copyright.html" },
        { name: "CGAL", license: "https://www.cgal.org/license.html" },
        { name: "Discord RPC", license: "https://github.com/discord/discord-rpc/blob/master/LICENSE" },
        { name: "GLAD", license: "https://github.com/Dav1dde/glad/blob/glad2/LICENSE" },
        { name: "libjpeg-turbo", license: "https://github.com/libjpeg-turbo/libjpeg-turbo/blob/main/LICENSE.md" },
        { name: "FreeType", license: "https://freetype.org/license.html" },
        { name: "ImGui", license: "https://www.dearimgui.com/licenses/" },
        { name: "ImPlot", license: "https://github.com/epezent/implot/blob/master/LICENSE" },
        { name: "libarchive", license: "https://raw.githubusercontent.com/libarchive/libarchive/master/COPYING" },
        { name: "libjxl", license: "https://github.com/libjxl/libjxl/blob/main/LICENSE" },
        { name: "libpng", license: "http://www.libpng.org/pub/png/src/libpng-LICENSE.txt" },
        { name: "lz4", license: "https://github.com/lz4/lz4/blob/dev/LICENSE" },
        { name: "microprofile", license: "https://github.com/jonasmr/microprofile/blob/master/LICENSE" },
        { name: "OpenSSL", license: "https://www.openssl.org/source/apache-license-2.0.txt" },
        { name: "opus", license: "https://opus-codec.org/license/" },
        { name: "PortAudio", license: "https://files.portaudio.com/docs/v19-doxydocs/License.html" },
        { name: "pugixml", license: "https://pugixml.org/license.html" },
        { name: "Qt 6", license: "https://doc.qt.io/qt-6/lgpl.html" },
        { name: "RakNet", license: "https://github.com/facebookarchive/RakNet/blob/master/LICENSE" },
        { name: "RapidJSON", license: "https://github.com/Tencent/rapidjson/blob/master/license.txt" },
        { name: "SDL3", license: "https://www.libsdl.org/license.php" },
        { name: "xxHash", license: "https://github.com/Cyan4973/xxHash/blob/dev/LICENSE" },
        { name: "zlib", license: "https://www.zlib.net/zlib_license.html" },
        { name: "zstd", license: "https://github.com/facebook/zstd/blob/dev/LICENSE" }
    ]

    const cardClass = "mx-4 my-2 p-4 rounded rounded-lg border border-slate-100 bg-gray-50 shadow dark:bg-neutral-900 dark:border-neutral-800"
    const iconClass = "me-1 [image-rendering:pixelated]"
    const statClass = "flex items-center text-stone-800 dark:text-stone-300 font-medium"

    function humanizeDuration(seconds) {
        const hours = Math.floor(seconds / 3600)
        const minutes = Math.floor((seconds % 3600) / 60)

        return minutes > 0 ? `${hours}h ${minutes}m` : `${minutes}m ${seconds % 60}s`
    }

    function humanizeTimestamp(timestamp) {
        const date = new Date(timestamp * 1000)

        return `${date.toDateString()} ${date.toTimeString().split(" ")[0]}`
    }

    onMount(async () => {
        if (!transport) return

        [platform, version, isUsingInstance, totalPlaytime, compiler, compileTimestamp, instanceUpdateState] = await Promise.all([
            transport.getPlatformName(),
            transport.getVersion(),
            transport.isUsingInstance(),
            transport.getTotalPlaytime().then(humanizeDuration),
            transport.getCompilerName(),
            transport.getCompileTime().then(humanizeTimestamp),
            transport.getInstanceUpdateState()
        ]); // this semicolon is very important... svelte's js parser breaks without it!

        [totalLevels, totalItems, totalModels] = await Promise.all([
            transport.getTotalLevels(),
            transport.getTotalItems(),
            transport.getTotalModels()
        ])

        if (isUsingInstance) {
            [instanceUrl, instanceName, instanceMotd] = await Promise.all([
                transport.getInstanceURL(),
                transport.getInstanceName(),
                transport.getInstanceMotd()
            ])
        }
    })
</script>

<div class="h-full flex flex-col overflow-hidden">
    <div class="{cardClass} flex items-center justify-between">
        <div class="flex items-center">
            <img alt="Aya" class="select-none pointer-events-none" src="./img/aya.png" width="80" />
            <div class="ms-5 flex flex-col">
                <span class="text-3xl mb-1.5 select-none"><b>Aya</b> for {platform ?? "Unknown"}</span>
                <code>{version ? `v${version}` : "N/A"}</code>
                <span class="text-sm"><span class="select-none">Compiled on</span><code class="ms-1">{compileTimestamp ?? "N/A"} ({compiler ?? "N/A"})</code></span>
            </div>
        </div>

        <div class="flex flex-col items-center pointer-events-none select-none space-y-0.5 mx-1">
            <span class={statClass}><img alt="Levels" class={iconClass} src="./img/icons/level.png">{totalLevels ?? 0} levels</span>
            <span class={statClass}><img alt="Items" class={iconClass} src="./img/icons/item.png">{totalItems ?? 0} items</span>
            <span class={statClass}><img alt="Models" class={iconClass} src="./img/icons/model.png">{totalModels ?? 0} models</span>
        </div>
    </div>

    {#if isUsingInstance}
        <div class="{cardClass} mb-2 flex items-center">
            <img alt="{instanceName}" class="select-none pointer-events-none" src="./img/small.png" width="80" />
            <div class="ms-5 flex flex-col">
                <span class="mb-1 text-3xl select-none font-bold">{instanceName ?? "Unknown"}</span>
                <div class="mb-1 flex items-center">
                    <span class="text-aya-500"><i class="fa-regular fa-globe me-1"></i><a href="{instanceUrl}" target="_blank" class="underline">{instanceUrl ?? "https://unknown.org/"}</a></span>
                    <i class="fa-regular fa-pipe"></i>
                    {#if instanceUpdateState && instanceUpdateState.enabled}
                        <span class="text-green-600 dark:text-green-400 font-semibold flex items-center"><i class="fa-solid fa-cloud fa-fw me-1"></i>Updates enabled <span class="text-xs ms-1">(last updated: {humanizeTimestamp(instanceUpdateState.lastUpdated)})</span></span>
                    {:else}
                        <span class="text-gray-500 dark:text-gray-400 flex items-center"><i class="fa-regular fa-cloud-slash fa-fw me-1"></i>Updates disabled</span>
                    {/if}
                </div>
                {#if instanceMotd}
                    <span class="text-sm italic">&ldquo;{instanceMotd}&rdquo;</span>
                {/if}
            </div>
        </div>
    {:else}
        <div class="flex flex-col items-center justify-center w-full select-none">
            <span class="text-gray-500 dark:text-gray-400 flex items-center mb-1"><i class="fa-regular fa-link-slash fa-fw me-1"></i>Not currently linked to an instance</span>
        </div>
    {/if}

    <div class="mb-2 flex flex-col items-center justify-center w-full select-none">
        <span class="text-stone-600 dark:text-neutral-200 font-semibold flex items-center"><i class="fa-regular fa-clock fa-fw me-1"></i>{totalPlaytime ?? "0m 0s"} of total playtime</span>
    </div>

    <div class="mb-2 flex items-center w-full justify-center flex-col select-none">
        <span class="text-3xl font-extrabold">License</span>
    </div>

    <div class="rounded-lg border border-neutral-200 dark:border-neutral-700 dark:bg-neutral-800/80 m-1.5 mb-4 flex flex-col flex-1 min-h-0">
        <div class="p-4 font-mono text-[12.5px] overflow-y-auto">
            <span class="font-semibold">Due to the fact that Aya is a fork of previously leaked proprietary Roblox source code, it is not possible to apply a traditional license to Aya. Therefore, all authors of custom non-proprietary code and related resources used in Aya hereby irrevocably waive all copyright and related rights to their contributions, to the maximum extent allowed by law.</span>
            <br />
            <br />Please note that this waiver of copyright applies only to the non proprietary items found in Aya. Any proprietary code that may still exist in Aya remains subject to the copyright and licensing conditions imposed by its original authors or owners; in particular, Roblox Corporation.
            <br />
            <br />Aya uses a variety of third party dependencies. A list of all the third party dependencies used in Aya alongside a link to their respective license are available below:
            <br />
            {#each dependencies as dep}
                <br /><i class="fa-sharp fa-dot mx-1"></i>{dep.name}: <a class="text-aya-500 underline" target="_blank" href="{dep.license}">{dep.license}</a>
            {/each}
            <br />
            <br />The authors of Aya have made a concerted effort to rid the original codebase of any and all proprietary or otherwise closed source dependencies, and to replace them with free and open source alternatives. The only proprietary or non-free items that still remain in this repository is code and artwork created by Roblox Corporation, which themselves have undergone substantial modification to the extent that they no longer resemble their original versions.
            <br />
            <br /><span class="underline">It is the duty of anyone who uses Aya to be fully aware of the legal circumstances surrounding its use. The original authors of Aya expressly disclaim all liability for any and all uses of Aya, including, without limitation, any direct, indirect, incidental, special, consequential, or exemplary damages, even if advised of the possibility of such damages. The original authors of Aya further disclaim any and all responsibility for any third party's use or misuse of Aya.</span>
            <br />
            <br /><b>THE MATERIALS IN THIS REPOSITORY, INCLUDING ALL SOURCE CODE AND OTHER RELATED ITEMS, SUCH AS DOCUMENTATION, ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR ANY OTHER COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.</b>
            <br />
            <br /><span class="italic">In addition to the legal responsibilities outlined above, we strongly encourage all users of Aya to use this software in a responsible and ethical manner. Please respect the rights and dignity of others, and use Aya only in a way that contributes positively to the world.</span>
        </div>
    </div>
</div>