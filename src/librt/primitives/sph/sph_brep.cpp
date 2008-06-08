/*                 S P H _ B R E P . C P P 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file breplicator.cpp
 *
 * Breplicator is a tool for testing the new boundary representation
 * (BREP) primitive type in librt.  It creates primitives via
 * mk_brep() for testing purposes.
 *
 * Author -
 *	Ben Poole
 */

#include "common.h"

#include "raytrace.h"
#include "rtgeom.h"
#include "wdb.h"
#include "bn.h"
#include "bu.h"

#include "opennurbs_circle.h"
#include "opennurbs_sphere.h"

void
rt_sph_brep(ON_Brep **b, const struct rt_db_internal *ip, const struct bn_tol *)
{
    struct rt_sph_internal *tip;
    point_t 

    RT_CK_DB_INTERNAL(ip);
    tip = (struct rt_ell_internal *)ip->idb_ptr;
    RT_ELL_CK_MAGIC(tip);
    
    ON_Sphere sph(tip->v, tip->a[0]);
    *b = ON_BrepSphere(sph);
}

// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
